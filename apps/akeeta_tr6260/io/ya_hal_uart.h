
#ifndef _YA_HAL_UART_H_
#define _YA_HAL_UART_H_


#define BAUD_RATE  115200

int32_t ya_hal_uart_open(void);

int32_t ya_hal_uart_write(uint8_t *buffer, uint32_t len);

int32_t ya_hal_uart_recv(uint8_t *buffer, uint32_t len, uint32_t timeout);


#endif


