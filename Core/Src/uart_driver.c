/*
 * uart_driver.c
 *
 *  Created on: Nov 16, 2021
 *      Author: jenni
 */

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "usart.h"

#include "uart_driver.h"

#define MIN_SEC_TO_MS(minutes, seconds) ((minutes)*60 * 1000 + (seconds)*1000)

uint32_t get_tick()
{
	return HAL_GetTick();
}

void send_msg(const char *msg)
{
	size_t msg_len = strlen(msg);
	HAL_UART_Transmit(&huart6, msg, msg_len, MIN_SEC_TO_MS(0, 5));
}



