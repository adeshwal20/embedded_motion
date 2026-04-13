#ifndef UART_RX_H
#define UART_RX_H

#include <stdbool.h>
#include "protocol.h"

void uart_rx_init(void);

// Returns true only when a fresh command byte was received.
bool uart_receive_command(command_t *out);

#endif