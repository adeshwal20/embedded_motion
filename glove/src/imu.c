#include "imu.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
<<<<<<< HEAD
=======
#include <stdbool.h>
>>>>>>> dbd89a8 (nothing' happening but communication)

#define IMU_I2C                 i2c0
#define IMU_BAUD                400000

#define IMU_SDA_PIN             24
#define IMU_SCL_PIN             25

// Scanner showed your IMU at 0x4A
#define BNO085_ADDR             0x4A

// SHTP channels
#define SHTP_CHAN_COMMAND       0
#define SHTP_CHAN_EXECUTABLE    1
#define SHTP_CHAN_CONTROL       2
#define SHTP_CHAN_REPORTS       3
#define SHTP_CHAN_WAKE_REPORTS  4
#define SHTP_CHAN_GYRO          5

// Commands / reports
#define SHTP_REPORT_SET_FEATURE     0xFD
#define SENSOR_REPORT_LINEAR_ACCEL  0x04

#define MAX_PACKET_SIZE         512

static bool imu_ready = false;
<<<<<<< HEAD
static uint8_t s_seq[6];
static imu_tilt_t s_last;
static bool s_fresh;

// SHTP channels we care about
#define SHTP_CH_CONTROL  2u
#define SHTP_CH_REPORTS  3u

// SH-2 IDs
#define SH2_SET_FEATURE  0xFDu
#define SH2_TIMEBASE     0xFBu
#define SH2_ACCEL        0x01u // Q8, m/s^2
#define SH2_GYRO_CAL     0x02u // Q9, rad/s

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
    // Header is 4 bytes, length includes header (bit15 = continuation, keep 0)
    uint16_t total_len = (uint16_t)(payload_len + 4u);
    if (total_len > 256u) {
        return false;
    }

    uint8_t buf[256];
    buf[0] = (uint8_t)(total_len & 0xFFu);
    buf[1] = (uint8_t)((total_len >> 8) & 0x7Fu);
    buf[2] = channel;
    buf[3] = s_seq[channel]++;
    if (payload_len) {
        memcpy(&buf[4], payload, payload_len);
    }

    int rc = i2c_write_blocking(IMU_I2C, BNO085_ADDR, buf, total_len, false);
    return rc == (int)total_len;
}

static int shtp_read_packet(uint8_t *out_channel, uint8_t *out_payload, uint16_t out_cap) {
    uint8_t hdr[4];
    int hr = i2c_read_blocking(IMU_I2C, BNO085_ADDR, hdr, (int)sizeof(hdr), false);
    if (hr != (int)sizeof(hdr)) {
        return -1;
    }

    uint16_t len = (uint16_t)(le16(hdr) & 0x7FFFu);
    uint8_t ch = hdr[2];
    // uint8_t seq = hdr[3]; // could be used for drop detection

    if (len < 4u) {
        return 0;
    }

    uint16_t payload_len = (uint16_t)(len - 4u);
    if (payload_len == 0u) {
        *out_channel = ch;
        return 0;
    }

    // Read only what we can store; drain the rest if needed.
    uint16_t to_read = payload_len;
    if (to_read > out_cap) {
        to_read = out_cap;
    }

    int pr = i2c_read_blocking(IMU_I2C, BNO085_ADDR, out_payload, (int)to_read, false);
    if (pr != (int)to_read) {
        return -1;
    }

    // Drain remainder (rare unless someone sends huge packets)
    uint16_t remaining = (uint16_t)(payload_len - to_read);
    while (remaining) {
        uint8_t junk[32];
        uint16_t chunk = remaining > sizeof(junk) ? (uint16_t)sizeof(junk) : remaining;
        int jr = i2c_read_blocking(IMU_I2C, BNO085_ADDR, junk, (int)chunk, false);
        if (jr != (int)chunk) {
            break;
        }
        remaining = (uint16_t)(remaining - chunk);
    }

    *out_channel = ch;
    return (int)to_read;
}

static void sh2_send_set_feature(uint8_t report_id, uint32_t interval_us) {
    // Set Feature (0xFD) payload is 17 bytes in the datasheet example:
    // [0]=0xFD, [1]=featureReportId, [2]=flags, [3..4]=changeSensitivity (0),
    // [5..8]=reportIntervalUs, [9..12]=batchIntervalUs (0), [13..16]=sensorSpecific (0)
    uint8_t p[17];
    memset(p, 0, sizeof(p));
    p[0] = SH2_SET_FEATURE;
    p[1] = report_id;
    p[2] = 0x00; // feature flags
    // p[3..4] change sensitivity = 0
    wr_le32(&p[5], interval_us);
    // batch interval + sensor specific remain 0
    (void)shtp_write(SHTP_CH_CONTROL, p, (uint16_t)sizeof(p));
}

static void sh2_parse_reports(const uint8_t *p, uint16_t n) {
    // Reports can be concatenated; we also sometimes see a timebase ref (0xFB).
    uint16_t i = 0;
    while (i < n) {
        uint8_t rid = p[i];
        if (rid == SH2_TIMEBASE) {
            // 0xFB + 4 bytes base delta
            if ((uint16_t)(n - i) < 5u) return;
            i = (uint16_t)(i + 5u);
            continue;
        }

        if (rid == SH2_ACCEL) {
            if ((uint16_t)(n - i) < 10u) return;
            int16_t x = le16s(&p[i + 4]);
            int16_t y = le16s(&p[i + 6]);
            int16_t z = le16s(&p[i + 8]);
            // Q8 => divide by 2^8
            s_last.ax = (float)x / 256.0f;
            s_last.ay = (float)y / 256.0f;
            s_last.az = (float)z / 256.0f;
            s_fresh = true;
            i = (uint16_t)(i + 10u);
            continue;
        }

        if (rid == SH2_GYRO_CAL) {
            if ((uint16_t)(n - i) < 10u) return;
            int16_t x = le16s(&p[i + 4]);
            int16_t y = le16s(&p[i + 6]);
            int16_t z = le16s(&p[i + 8]);
            // Q9 => divide by 2^9
            s_last.gx = (float)x / 512.0f;
            s_last.gy = (float)y / 512.0f;
            s_last.gz = (float)z / 512.0f;
            s_fresh = true;
            i = (uint16_t)(i + 10u);
            continue;
        }

        // Unknown report: we don't know its length; bail out to avoid desync.
        return;
    }
=======
static uint8_t seq_nums[6] = {0};

static int imu_i2c_read(uint8_t *dst, size_t len) {
    return i2c_read_blocking(IMU_I2C, BNO085_ADDR, dst, len, false);
}

static int imu_i2c_write(const uint8_t *src, size_t len) {
    return i2c_write_blocking(IMU_I2C, BNO085_ADDR, src, len, false);
}

static uint16_t read_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static int16_t read_i16_le(const uint8_t *p) {
    return (int16_t)read_u16_le(p);
}

static bool shtp_write_packet(uint8_t channel, const uint8_t *payload, uint16_t payload_len) {
    uint8_t tx[MAX_PACKET_SIZE];
    uint16_t total_len = payload_len + 4;

    if (total_len > MAX_PACKET_SIZE) {
        printf("write packet too large: %u\n", total_len);
        return false;
    }

    tx[0] = (uint8_t)(total_len & 0xFF);
    tx[1] = (uint8_t)((total_len >> 8) & 0x7F);
    tx[2] = channel;
    tx[3] = seq_nums[channel]++;

    memcpy(&tx[4], payload, payload_len);

    int ret = imu_i2c_write(tx, total_len);
    if (ret != total_len) {
        printf("write packet failed ret=%d expected=%u\n", ret, total_len);
        return false;
    }

    return true;
}

static bool shtp_read_packet(uint8_t *buf, uint16_t *packet_len, uint8_t *channel) {
    uint8_t header[4];
    int ret = imu_i2c_read(header, 4);

    if (ret != 4) {
        printf("header read ret=%d\n", ret);
        return false;
    }

    uint16_t len = (uint16_t)header[0] | ((uint16_t)header[1] << 8);
    len &= 0x7FFF;

    printf("raw header: %02X %02X %02X %02X len=%u\n",
           header[0], header[1], header[2], header[3], len);

    if (len < 4 || len > MAX_PACKET_SIZE) {
        printf("bad len=%u\n", len);
        return false;
    }

    buf[0] = header[0];
    buf[1] = header[1];
    buf[2] = header[2];
    buf[3] = header[3];

    if (len > 4) {
        ret = imu_i2c_read(&buf[4], len - 4);
        if (ret != (int)(len - 4)) {
            printf("payload read ret=%d expected=%u\n", ret, len - 4);
            return false;
        }
    }

    *packet_len = len;
    *channel = buf[2];
    return true;
}

static bool bno_enable_linear_accel(uint32_t interval_us) {
    uint8_t payload[17] = {0};

    // Set Feature command
    payload[0] = SHTP_REPORT_SET_FEATURE;
    payload[1] = SENSOR_REPORT_LINEAR_ACCEL;

    payload[5] = (uint8_t)(interval_us & 0xFF);
    payload[6] = (uint8_t)((interval_us >> 8) & 0xFF);
    payload[7] = (uint8_t)((interval_us >> 16) & 0xFF);
    payload[8] = (uint8_t)((interval_us >> 24) & 0xFF);

    printf("sending set feature for linear accel\n");
    return shtp_write_packet(SHTP_CHAN_CONTROL, payload, sizeof(payload));
>>>>>>> dbd89a8 (nothing' happening but communication)
}

bool imu_init(void) {
    i2c_init(IMU_I2C, IMU_BAUD);

    gpio_set_function(IMU_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(IMU_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(IMU_SDA_PIN);
    gpio_pull_up(IMU_SCL_PIN);

    sleep_ms(500);

<<<<<<< HEAD
    memset(s_seq, 0, sizeof(s_seq));
    memset(&s_last, 0, sizeof(s_last));
    s_fresh = false;

    // Drain a few startup packets (advertisement/reset chatter)
    for (int k = 0; k < 8; k++) {
        uint8_t ch = 0;
        uint8_t payload[128];
        (void)shtp_read_packet(&ch, payload, (uint16_t)sizeof(payload));
        sleep_ms(5);
    }

    // Enable accel + calibrated gyro at 100 Hz (10,000 us)
    sh2_send_set_feature(SH2_ACCEL, 10000u);
    sh2_send_set_feature(SH2_GYRO_CAL, 10000u);

    imu_ready = true;
    return true;
=======
    bool ok = bno_enable_linear_accel(50000);
    printf("enable linear accel: %s\n", ok ? "ok" : "failed");

    sleep_ms(500);

    imu_ready = ok;
    return ok;
>>>>>>> dbd89a8 (nothing' happening but communication)
}

bool imu_read_tilt(imu_tilt_t *out) {
    if (!imu_ready || !out) {
        return false;
    }

<<<<<<< HEAD
    // Poll a couple packets each call (no INT pin in this project yet).
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
=======
    uint8_t packet[MAX_PACKET_SIZE];
    uint16_t len = 0;
    uint8_t channel = 0;

    for (int tries = 0; tries < 10; tries++) {
        if (!shtp_read_packet(packet, &len, &channel)) {
            printf("read packet failed\n");
            continue;
        }

        printf("len=%u chan=%u", len, channel);

        if (len > 4) {
            printf(" payload:");
            for (int i = 4; i < len && i < 12; i++) {
                printf(" %02X", packet[i]);
            }
        }
        printf("\n");

        if (channel != SHTP_CHAN_REPORTS) {
            continue;
        }

        if (len < 14) {
            continue;
        }

        const uint8_t *p = &packet[4];

        if (p[0] != SENSOR_REPORT_LINEAR_ACCEL) {
            continue;
        }

        int16_t raw_x = read_i16_le(&p[4]);
        int16_t raw_y = read_i16_le(&p[6]);
        int16_t raw_z = read_i16_le(&p[8]);

        out->ax = (float)raw_x / 256.0f;
        out->ay = (float)raw_y / 256.0f;
        out->az = (float)raw_z / 256.0f;

        return true;
    }

    return false;
>>>>>>> dbd89a8 (nothing' happening but communication)
}