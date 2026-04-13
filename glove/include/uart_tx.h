#ifndef UART_TX_H
#define UART_TX_H

#include "protocol.h"

void uart_tx_init(void);
void uart_send_command(command_t cmd);

#endif