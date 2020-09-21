#ifndef _SMARTCONFIG_H_
#define _SMARTCONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "stdint.h"

#ifndef MAC_FMT
#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#ifndef MAC_ARG
#define MAC_ARG(x) ((uint8_t*)(x))[0],((uint8_t*)(x))[1],((uint8_t*)(x))[2],((uint8_t*)(x))[3],((uint8_t*)(x))[4],((uint8_t*)(x))[5]
#endif

#define PRE_TIMES                               3


#define CAL_ASCII_BROADCAST(len, even)                 (len-256*even) //偶数-512
#define SCONFIG_DATA_SIZE                              4
#define SCONFIG_DATA_NUM                               1
#define RESET_KEY               0xaa

#define MAX_SSID_LEN     (32 + 1)/* ssid: 32 octets at most, include the NULL-terminated */
#define MAX_PASSWD_LEN   (64 + 1)/* 8-63 ascii */



typedef struct router_info {
    struct router_info    *next;
	uint8_t auth;
	uint8_t channel;
	uint8_t encry[2];
	uint8_t bssid[6];
	char ssid[MAX_SSID_LEN];
}ROUTER_INFO;

typedef enum
{
     SCONFIG_IDLE,
    SCONFIG_LEN,
    SCONFIG_INDEX,
    SCONFIG_KEY,
    SCONFIG_CHK,
    SCONFIG_OK,
}SCONFIG_STATUS;

typedef enum
{
    SCONFIG_VALUE_IDLE = 0,
    SCONFIG_VALUE_SOH = 1,
    SCONFIG_VALUE_STX = 7,
    SCONFIG_VALUE_BELL = 14,
    SCONFIG_VALUE_ETX = 20,
}SCONFIG_VALUE;


enum
{
    CHANNEL_SEARCH_START = 0,
    CHANNEL_SEARCH_END,
    CHANNEL_SEARCH_OK,
};
enum
{
    NOT_ACTIVE = 0,
    OK_KEY,
};
enum
{
    PKT_TYPE_SSID = 0,
    PKT_TYPE_KEY,
};

typedef struct sconfig_info
{
    uint8_t switchChannel;  /*信道*/   
    uint8_t findPhone;         
    uint16_t delay;         /*切信道时间*/                
    uint8_t bssid[6];
    uint8_t srcmac[6];
}SCONFIG_INFO;

#ifdef __cplusplus
}
#endif
#endif