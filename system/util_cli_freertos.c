#include "system.h"

#if !defined(RELEASE)
#define CLI_HISTORY
#endif

#ifndef AMT
#define MAX_INPUT_LENGTH (NRC_MAX_CMDLINE_SIZE)
#else
#define MAX_INPUT_LENGTH 1152
extern int global_amt_uboot_flag;
#endif
#define IS_ASCII(c) (c > 0x1F && c < 0x7F)

#ifdef CLI_HISTORY
#define MAX_HISTORY 10
char g_cli_history[MAX_HISTORY][MAX_INPUT_LENGTH];
uint8_t g_cli_history_index, g_cli_history_rd_index = 0;
bool g_cli_history_escape = false;
#endif
extern uint8_t ya_atcmd_start_flag;
#define MAX_INPUT_LENGTH_ATCMD (8*1024)

//static char *(*g_cli_prompt_func)();
static int (*g_cli_run_command)(char*);
bool g_cli_typing;

static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
		StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
	*ppxTimerTaskTCBBuffer		= &xTimerTaskTCBBuffer;
	*ppxTimerTaskStackBuffer	= &xTimerStack[0];
	*pulTimerTaskStackSize		= configTIMER_TASK_STACK_DEPTH;
}

static void print_prompt()
{
#ifndef UART_WPA_SEPARATION
	static int line;
	system_printf("[%d]%s:", line++, TARGET_NAME);
#endif
}
static int8_t reMalloc_flag = 0;
struct {
	char *buffer;
	int index;
}cli = {0};

static int32_t cli_handler(void *arg)
{
	char *str = arg;
	if(ya_atcmd_start_flag == 0)
	{
		if(g_cli_run_command) 
		g_cli_run_command(str);
	vPortFree(str);
	print_prompt();
	}
	else
	{
		if(g_cli_run_command) 
			g_cli_run_command(cli.buffer);
		cli.index = 0;		
	}
	return 0;
}

#if 0
uint8_t dump_cur_task(char *cmd)
{
    TaskHandle_t task;
    
    if (!cmd || cmd[0] == '\0')
        return -1;
    if (strcmp(cmd, "show task"))
        return 0;

    task = xTaskGetCurrentTaskHandle();
    system_printf("Current Task: %s\n", pcTaskGetName(task));

	vPortFree(cmd);
	print_prompt();
    return 1;
}
#endif
void util_cli_callback(char c)
{
	if(ya_atcmd_start_flag == 0)
	{
		if (!cli.buffer) 
		{
		cli.buffer = pvPortMalloc(MAX_INPUT_LENGTH);
		cli.index = 0;
		ASSERT(cli.buffer);
		}		
	}
	else
	{
		if(reMalloc_flag == 0)
		{
			reMalloc_flag = 1;
			if (cli.buffer)
			{
				vPortFree(cli.buffer);
				cli.buffer = NULL;
			}
			if (!cli.buffer) 
			{
				cli.buffer = pvPortMalloc(MAX_INPUT_LENGTH_ATCMD);
				cli.index = 0;
				ASSERT(cli.buffer);
			}					
		}		
	}
	if(ya_atcmd_start_flag == 0)
	{
#ifdef CLI_HISTORY
	/// To command history
	if (c == ESCAPE_KEY) {
		int i;
		for (i = 0; i < cli.index; i++) // move cursor to next of prompt
			system_oprintf("\b", 1);
		for (i = 0; i < MAX_INPUT_LENGTH; i++) 	// delete all char in console
			system_oprintf(" ", 1);
		for (i = 0; i < MAX_INPUT_LENGTH; i++) // move cursor to next of prompt
			system_oprintf("\b", 1);

		cli.index = 0;
		cli.buffer[cli.index++] = c;
		g_cli_history_escape = true;
		return;
	}

	//show history
	if (g_cli_history_escape) {
		cli.buffer[cli.index++] = c;
		if (cli.index == 3) {
			g_cli_history_escape = false;
			if (cli.buffer[0] == 27 && cli.buffer[1] == 91 && cli.buffer[2] == 65) {
				int i;
				for (i = 0; i < MAX_HISTORY; i++) {
					if (g_cli_history_rd_index == 0)
						g_cli_history_rd_index = MAX_HISTORY - 1;
					else
						g_cli_history_rd_index--;

					if (g_cli_history[g_cli_history_rd_index][0] != '\0')
						break;
				}
				char *history = &g_cli_history[g_cli_history_rd_index][0];
				system_printf("%s", history);
				cli.index = strlen(history);
				memcpy(cli.buffer, history, cli.index);
			} else {
				cli.index = 0;
			}
		}
		return;
	}

	g_cli_history_escape = false;

	if(c == RETURN_KEY) {
		if (cli.index > 0) {
				if(ya_atcmd_start_flag == 0)
			memset(&g_cli_history[g_cli_history_index][0], 0, MAX_INPUT_LENGTH);
				else
					memset(&g_cli_history[g_cli_history_index][0], 0, MAX_INPUT_LENGTH_ATCMD);
			memcpy(&g_cli_history[g_cli_history_index][0], cli.buffer, cli.index);
			if (++g_cli_history_index == MAX_HISTORY)
				g_cli_history_index = 0;

			g_cli_history_rd_index = g_cli_history_index;
		}
	}
#endif
	}

	switch(c) {
		case BACKSP_KEY:
			if(ya_atcmd_start_flag == 0)
			{
			if (cli.index > 0) {
				system_oprintf("\b \b", 3);
				cli.index--;
				}				
			}
			break;
		case RETURN_KEY: 
			if(ya_atcmd_start_flag == 0)
			{
			g_cli_typing = !g_cli_typing;
#ifdef UART_WPA_SEPARATION
			system_oprintf("\n", 1);
#else
#ifndef AMT
			system_printf("\n");
#endif
#endif
			}
			if (cli.index > 0) {
				ASSERT(cli.buffer);
				cli.buffer[cli.index] = '\0';
				//if (!dump_cur_task(cli.buffer)) {
					if (false == system_schedule_work_queue_from_isr(cli_handler, cli.buffer, NULL)) {
                    	vPortFree(cli.buffer);
						if(ya_atcmd_start_flag == 0)
	                    print_prompt();
                    }
                //}
				if(ya_atcmd_start_flag == 0)
				cli.buffer = NULL;
			} else {
				if(ya_atcmd_start_flag == 0)
				print_prompt();
			}
			break;
		if(ya_atcmd_start_flag == 0)
		{
		case '\n':
			break;
		}
		default:
		if(ya_atcmd_start_flag == 0)
		{
			if (IS_ASCII(c) &&
				cli.index < MAX_INPUT_LENGTH) {
				g_cli_typing = true;
				cli.buffer[cli.index++] = c;
				//cli.buffer[cli.index] = '\0';
				#ifndef AMT
				system_oprintf(&c, 1);
				#else
				if(!global_amt_uboot_flag)
					system_oprintf(&c, 1);
				#endif
				//uart_data_write(UART0_BASE, (const unsigned char *)&c, 1);
			}
		}
		else
		{
			if (IS_ASCII(c) &&
				cli.index < MAX_INPUT_LENGTH_ATCMD) 
			{
				cli.buffer[cli.index++] = c;
			}				
		}
			break;
	}
}

bool util_cli_freertos_init( int (*run_command)(char *))
{
//    g_cli_prompt_func = prompt_func;
    g_cli_run_command = run_command;

    return true;
}
