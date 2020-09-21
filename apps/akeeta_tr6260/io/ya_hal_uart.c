#include "ya_common.h"
#include "ya_hal_os.h"
#include "drv_uart.h"

static uart_handle_t ya_uart_handle=NULL;
static ya_hal_os_thread_t  ya_uart_read_thread_handle=NULL;
static ya_hal_os_mutex_t Semaphore_uart_read;

extern SemaphoreHandle_t xCountingSemaphore_uart;

#define MAX_UART_BUFFER_DATA 		1024
uint8_t uart_data_buffer[MAX_UART_BUFFER_DATA];
uint16_t uart_data_len = 0;

static void ya_uart_rx_task()
{ 	
	int32_t i;
	int32_t total_len;
	 while(1)
	 {	 
	 	if(xSemaphoreTake(xCountingSemaphore_uart, portMAX_DELAY) == pdTRUE)
		{
			total_len = hal_uart_get_rx_len();
			if (total_len && (uart_data_len+total_len)<MAX_UART_BUFFER_DATA)
			{
				memcpy(uart_data_buffer + uart_data_len, hal_uart_get_rx_buff(), total_len);
				uart_data_len += total_len;
			}
			hal_uart_get_rx_data_end();

			if(total_len == 0)
				ya_hal_os_up_sema(&Semaphore_uart_read);							
		}
	}		 
}
int32_t ya_hal_uart_open()
{
	int32_t ret;
	if(ya_uart_handle == NULL)
		ya_uart_handle = hal_uart_open_data(1, UART_ID_1, UART_DATA_BIT_8, UART_BAUD_RATE_115200, UART_PARITY_NONE, UART_STOP_BIT_1, 0);
	if(ya_uart_handle == NULL)
		return -1;		
	ya_hal_os_init_sema(&Semaphore_uart_read, 0);
	ret = ya_hal_os_thread_create(&ya_uart_read_thread_handle, "ya_uart_read_thread", ya_uart_rx_task, 0, 1024, 2);
	if(ret != 0)
	{
		return -1;
	}
	return 0;
}

int32_t ya_hal_uart_write(uint8_t *buffer, uint32_t len)
{
	int32_t ret = 0;
	if(ya_uart_handle == NULL || !len || !buffer)
		return -1;
	hal_uart_write(ya_uart_handle, buffer , len);
	return 1;
}

int32_t ya_hal_uart_recv(uint8_t *buffer, uint32_t len, uint32_t timeout)
{
	uint16_t index = 0;
	if(!len || !buffer)
	{
		return -1;
	}
	if(ya_hal_os_down_sema(&Semaphore_uart_read,timeout) == pdTRUE)
	{
		if(uart_data_len >= len)
		{
			memcpy(buffer, uart_data_buffer, len);
			for(index = 0; index < (MAX_UART_BUFFER_DATA - len); index++)
			{
				uart_data_buffer[index] = uart_data_buffer[index + len];
			}
			
			uart_data_len -= len;
			return len;
		}

	}
	return 0;	
}

