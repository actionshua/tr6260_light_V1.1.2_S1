
#ifndef _YA_COMMOM_FUNC_H_
#define _YA_COMMOM_FUNC_H_

extern void HexArrayToString(uint8_t *hexarray, uint16_t length, char *string);

extern int StringToHexArray(uint8_t *hexarray,uint16_t length, char *string);

extern char *ya_int_to_string(uint32_t value);

#endif

