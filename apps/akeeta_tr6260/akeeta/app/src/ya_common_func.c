/*
 * Copyright(c) 2018 - 2020 Yaguan Technology Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "ya_common.h"

const unsigned char Num2CharTable[16] = "0123456789ABCDEF";

void HexArrayToString(uint8_t *hexarray, uint16_t length, char *string)
{
    int i = 0;
	while(i < length)
	{
	    *(string++) = Num2CharTable[(hexarray[i] >> 4) & 0x0f];
		*(string++) = Num2CharTable[hexarray[i] & 0x0f];
		i++;
	}
	*string = 0x0;
}

int StringToHexArray(uint8_t *hexarray,uint16_t length, char *string)
{
	uint16_t index = 0, array_index = 0, total_len = 0;
	uint8_t value = 0;

	total_len = strlen(string);
	if ((2*length) < total_len)
		return -1;

	for(index = 0; index < total_len; index++)
	{
		if (string[index] >= 'a' && string[index] <= 'f')
			value = string[index] - 'a' + 10;
		else if (string[index] >= 'A' && string[index] <= 'F')
			value = string[index] - 'A' + 10;
		else if (string[index] >= '0' && string[index] <= '9')
			value = string[index] - '0';
		else
			return -1;
			
		if (index % 2 == 0)
		{
			hexarray[array_index] = (value << 4);
		}else
		{
			hexarray[array_index] += value;
			array_index++;
		}
	}

	return array_index;
}


char *ya_int_to_string(uint32_t value)
{
	static char change_string[33];

	memset(change_string, 0, 33);
	snprintf(change_string, 33, "%d", value);
	return change_string;
}

