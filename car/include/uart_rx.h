#ifndef UART_RX_H
#define UART_RX_H

#include <stdbool.h>
#include "protocol.h"

void uart_rx_init(void);

// Returns true only when a fresh command byte was received.
// If which_uart is non-NULL: 0 = UART on GPIO 0/1, 1 = UART on GPIO 20/21.
bool uart_receive_command(command_t *out, unsigned *which_uart);

#endif