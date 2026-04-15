#include "imu.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define IMU_BAUD     100000

#define BNO085_ADDR_DEFAULT  0x4A

static uint8_t s_bno_addr = BNO085_ADDR_DEFAULT;
static i2c_inst_t *s_i2c = i2c1;
static uint8_t s_sda_pin = 26;
static uint8_t s_scl_pin = 27;

// SHTP channels
#define SHTP_CH_CONTROL  2u
#define SHTP_CH_REPORTS  3u

// SH-2 report IDs
#define SH2_SET_FEATURE  0xFDu
#define SH2_TIMEBASE     0xFBu
#define SH2_ACCEL        0x01u // Q8, m/s^2
#define SH2_GYRO_CAL     0x02u // Q9, rad/s

static bool imu_ready = false;
static uint8_t s_seq[6];
static imu_tilt_t s_last;
static bool s_fresh;

typedef struct {
    i2c_inst_t *bus;
    uint8_t sda;
    uint8_t scl;
} imu_i2c_candidate_t;

static const imu_i2c_candidate_t k_i2c_candidates[] = {
    {i2c1, 26, 27},
};

static bool probe_bno_addr(i2c_inst_t *bus, uint8_t addr) {
    return i2c_write_blocking(bus, addr, NULL, 0, false) >= 0;
}

static inline uint16_t le16(const uint8_t *p) {
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}

static inline int16_t le16s(const uint8_t *p) {
    return (int16_t)le16(p);
}

static inline void wr_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static bool shtp_write(uint8_t channel, const uint8_t *payload, uint16_t payload_len) {
    uint16_t total_len = (uint16_t)(payload_len + 4u);
    if (total_len > 256u) {
        return false;
    }

    uint8_t buf[256];
    buf[0] = (uint8_t)(total_len & 0xFFu);
    buf[1] = (uint8_t)((total_len >> 8) & 0x7Fu);
    buf[2] = channel;
    buf[3] = s_seq[channel]++;
    if (payload_len != 0u) {
        memcpy(&buf[4], payload, payload_len);
    }

    int rc = i2c_write_blocking(s_i2c, s_bno_addr, buf, total_len, false);
    return rc == (int)total_len;
}

static int shtp_read_packet(uint8_t *out_channel, uint8_t *out_payload, uint16_t out_cap) {
    uint8_t hdr[4];
    int hr = i2c_read_blocking(s_i2c, s_bno_addr, hdr, (int)sizeof(hdr), false);
    if (hr != (int)sizeof(hdr)) {
        return -1;
    }

    uint16_t len = (uint16_t)(le16(hdr) & 0x7FFFu);
    uint8_t ch = hdr[2];

    if (len < 4u) {
        return 0;
    }

    uint16_t payload_len = (uint16_t)(len - 4u);
    if (payload_len == 0u) {
        *out_channel = ch;
        return 0;
    }

    uint16_t to_read = payload_len > out_cap ? out_cap : payload_len;
    int pr = i2c_read_blocking(s_i2c, s_bno_addr, out_payload, (int)to_read, false);
    if (pr != (int)to_read) {
        return -1;
    }

    uint16_t remaining = (uint16_t)(payload_len - to_read);
    while (remaining != 0u) {
        uint8_t junk[32];
        uint16_t chunk = remaining > sizeof(junk) ? (uint16_t)sizeof(junk) : remaining;
        int jr = i2c_read_blocking(s_i2c, s_bno_addr, junk, (int)chunk, false);
        if (jr != (int)chunk) {
            break;
        }
        remaining = (uint16_t)(remaining - chunk);
    }

    *out_channel = ch;
    return (int)to_read;
}

static void sh2_send_set_feature(uint8_t report_id, uint32_t interval_us) {
    // Set Feature command payload: 17 bytes
    uint8_t p[17];
    memset(p, 0, sizeof(p));
    p[0] = SH2_SET_FEATURE;
    p[1] = report_id;
    p[2] = 0x00; // feature flags
    wr_le32(&p[5], interval_us);
    (void)shtp_write(SHTP_CH_CONTROL, p, (uint16_t)sizeof(p));
}

static void sh2_parse_reports(const uint8_t *p, uint16_t n) {
    uint16_t i = 0;
    while (i < n) {
        uint8_t rid = p[i];

        if (rid == SH2_TIMEBASE) {
            if ((uint16_t)(n - i) < 5u) {
                return;
            }
            i = (uint16_t)(i + 5u);
            continue;
        }

        if (rid == SH2_ACCEL) {
            if ((uint16_t)(n - i) < 10u) {
                return;
            }
            int16_t x = le16s(&p[i + 4]);
            int16_t y = le16s(&p[i + 6]);
            int16_t z = le16s(&p[i + 8]);
            s_last.ax = (float)x / 256.0f;
            s_last.ay = (float)y / 256.0f;
            s_last.az = (float)z / 256.0f;
            s_fresh = true;
            i = (uint16_t)(i + 10u);
            continue;
        }

        if (rid == SH2_GYRO_CAL) {
            if ((uint16_t)(n - i) < 10u) {
                return;
            }
            int16_t x = le16s(&p[i + 4]);
            int16_t y = le16s(&p[i + 6]);
            int16_t z = le16s(&p[i + 8]);
            s_last.gx = (float)x / 512.0f;
            s_last.gy = (float)y / 512.0f;
            s_last.gz = (float)z / 512.0f;
            s_fresh = true;
            i = (uint16_t)(i + 10u);
            continue;
        }

        return;
    }
}

bool imu_init(void) {
    bool found = false;
    for (uint i = 0; i < (sizeof(k_i2c_candidates) / sizeof(k_i2c_candidates[0])); i++) {
        const imu_i2c_candidate_t *c = &k_i2c_candidates[i];
        i2c_init(c->bus, IMU_BAUD);
        gpio_set_function(c->sda, GPIO_FUNC_I2C);
        gpio_set_function(c->scl, GPIO_FUNC_I2C);
        gpio_pull_up(c->sda);
        gpio_pull_up(c->scl);
        sleep_ms(20);

        if (probe_bno_addr(c->bus, 0x4A) || probe_bno_addr(c->bus, 0x4B)) {
            s_i2c = c->bus;
            s_sda_pin = c->sda;
            s_scl_pin = c->scl;
            s_bno_addr = probe_bno_addr(c->bus, 0x4A) ? 0x4A : 0x4B;
            found = true;
            break;
        }
    }

    if (!found) {
        imu_ready = false;
        return false;
    }

    printf("IMU online: bus=%s SDA=%u SCL=%u addr=0x%02X\n",
           s_i2c == i2c0 ? "i2c0" : "i2c1",
           s_sda_pin, s_scl_pin, s_bno_addr);

    memset(s_seq, 0, sizeof(s_seq));
    memset(&s_last, 0, sizeof(s_last));
    s_fresh = false;

    // Drain startup chatter packets.
    for (int k = 0; k < 8; k++) {
        uint8_t ch = 0;
        uint8_t payload[128];
        (void)shtp_read_packet(&ch, payload, (uint16_t)sizeof(payload));
        sleep_ms(5);
    }

    // 100 Hz accel and gyro.
    sh2_send_set_feature(SH2_ACCEL, 10000u);
    sh2_send_set_feature(SH2_GYRO_CAL, 10000u);

    imu_ready = true;
    return true;
}

bool imu_read_tilt(imu_tilt_t *out) {
    if (!imu_ready || out == NULL) {
        return false;
    }

    for (int k = 0; k < 2; k++) {
        uint8_t ch = 0;
        uint8_t payload[128];
        int n = shtp_read_packet(&ch, payload, (uint16_t)sizeof(payload));
        if (n > 0 && ch == SHTP_CH_REPORTS) {
            sh2_parse_reports(payload, (uint16_t)n);
        }
    }

    if (!s_fresh) {
        return false;
    }

    *out = s_last;
    s_fresh = false;
    return true;
}
