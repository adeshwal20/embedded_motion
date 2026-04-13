#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

typedef enum {
    CMD_STOP = 0,
    CMD_FORWARD = 1,
    CMD_BACKWARD = 2,
    CMD_LEFT = 3,
    CMD_RIGHT = 4
} command_t;

static inline uint8_t command_to_byte(command_t cmd) {
    return (uint8_t)cmd;
}

static inline command_t byte_to_command(uint8_t b) {
    switch (b) {
        case CMD_FORWARD:  return CMD_FORWARD;
        case CMD_BACKWARD: return CMD_BACKWARD;
        case CMD_LEFT:     return CMD_LEFT;
        case CMD_RIGHT:    return CMD_RIGHT;
        default:           return CMD_STOP;
    }
}

#endif