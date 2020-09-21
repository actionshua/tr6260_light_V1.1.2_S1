/*******************************************************************************
 * Copyright by Transa Semi.
 *
 * File Name:    
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       V1.0
 * Author:        lixiao
 * Date:          2019-01-30
 * History 1:      
 *     Date: 
 *     Version:
 *     Author: 
 *     Modification:  
 * History 2: 
  ********************************************************************************/

/****************************************************************************
* 	                                           Include files
****************************************************************************/
#include "system.h"
#include "util_cmd.h"
#include "system_wifi.h"
#include "system_network.h"
#include "system_debug.h"

/****************************************************************************
* 	                                           Local Macros
****************************************************************************/
#define DUMP_PKT_FILTER_RULE_MAX            (4)

/****************************************************************************
* 	                                           Local Types
****************************************************************************/
typedef struct {
	uint8_t valid;
	uint8_t byte_index;
	uint8_t value;
} dump_pkt_filter_t;

typedef struct {
	uint8_t enable;
    uint8_t dir;
	dump_pkt_filter_t filter[DUMP_PKT_FILTER_RULE_MAX];
} dump_pkt_cfg_t;

typedef struct {
	uint8_t enable;
    uint8_t dir;
    uint8_t type;
    uint16_t stype;
} dump_frame_cfg_t;

static dump_frame_cfg_t dump_frame_cfg = {0};
static dump_pkt_cfg_t   dump_pkt_cfg = {0};

/****************************************************************************
* 	                                           Local Constants
****************************************************************************/

/****************************************************************************
* 	                                           Local Function Prototypes
****************************************************************************/

/****************************************************************************
* 	                                          Global Constants
****************************************************************************/

/****************************************************************************
* 	                                          Global Variables
****************************************************************************/

/****************************************************************************
* 	                                          Global Function Prototypes
****************************************************************************/

/****************************************************************************
* 	                                          Function Definitions
****************************************************************************/

#if 0
void dump_pkt_help(void)
{
	system_printf("\tusage:\ndump_pkt [enable/disable]\ndump_pkt filter [index] [byte] [value]\n\n");
    system_printf("\tusage:\ndump_frame [enable/disable]\ndump_frame type|stype value\n\n");
	return;
}

void show_dump_pkt_filter(void)
{
	int i;
    dump_pkt_filter_t *filter;

	system_printf("----pkt dump %s|dir %d.\n", dump_pkt_cfg.enable ? "enable" : "disable",
        dump_pkt_cfg.dir);
	for (i = 0; i < DUMP_PKT_FILTER_RULE_MAX; ++i) {
        filter = &dump_pkt_cfg.filter[i];
		if (!filter->valid)
			continue;
		system_printf("rule[%d]: byte[%d], value[0x%02x]\n", i, filter->byte_index,
			filter->value);
	}
}

void set_dump_pkt_filter(unsigned char index, unsigned char byte, unsigned char value)
{
    dump_pkt_filter_t *filter;
    
	if (index >= DUMP_PKT_FILTER_RULE_MAX)
		return dump_pkt_help();

    filter = &dump_pkt_cfg.filter[index];
	filter->valid = 1;
	filter->byte_index = byte;
	filter->value = value;
}

void dump_pkt_config(int argc, char **argv)
{
	unsigned char index, byte_index, value;
	
	if (argc <= 1) {
		return dump_pkt_help();
	}
	
	if (argc == 2) {
		if (!strcasecmp(argv[1], "enable")) {
			dump_pkt_cfg.enable = 1;
		} else if (!strcasecmp(argv[1], "disable")) {
			dump_pkt_cfg.enable = 0;
		} else if (!strcasecmp(argv[1], "clear")) {
			memset(dump_pkt_cfg.filter, 0, sizeof(dump_pkt_cfg.filter));
            return;
		}  else if (!strcasecmp(argv[1], "show")) {
			return show_dump_pkt_filter();
		} else {
			return dump_pkt_help();
		}
		system_printf("----%s pkt dump.\n", dump_pkt_cfg.enable ? "enable" : "disable");
		return;
	}

    if (!strcasecmp(argv[1], "dir") && argc == 3) {
	    dump_pkt_cfg.dir = atoi(argv[2]);
        return;
    }
    
	if (5 != argc) {
		return dump_pkt_help();
	}

	if (strcasecmp(argv[1], "filter")) {
		return dump_pkt_help();
	}

	index = atoi(argv[2]);
	byte_index = atoi(argv[3]);
	value = atoi(argv[4]);
    set_dump_pkt_filter(index, byte_index, value);
	return;
}

static inline int do_pkt_filter(unsigned char *data, int len, debug_rx_tx_info_t *info)
{
	int i, match = 1;
    dump_pkt_filter_t *filter;
	
	if (!dump_pkt_cfg.enable)
		return 0;

    if (!(dump_pkt_cfg.dir & (1 << info->dir)))
        return 0;
    
	for (i = 0; i < DUMP_PKT_FILTER_RULE_MAX; ++i) {
        filter = &dump_pkt_cfg.filter[i];
		if (!filter->valid)
			continue;
		if (len <= filter->byte_index) {
			match = 0;
			break;
		}
		if (data[filter->byte_index] != filter->value)
			match = 0;
	}
	return match;
}

int dump_content(unsigned char *data, int len, int offset)
{
	int i = 0;

	for (i = 0; i < len; i++) {
		if (offset % 16 == 0)
			system_printf("\n%06x    ", offset);
		else if (offset != 0 && offset % 8 == 0)
			system_printf("   ");
		system_printf("%02x ", data[i]);
		offset++;
	}
	return offset;
}

/*******************************************************************************
 * Function: dump_pbuf
 * Description: dump a pkt which match filter.
 * Parameters: 
 *   Input: pbuf to dump, rx or tx.
 *
 *   Output:
 *
 * Returns: 
 *
 *
 * Others: 
 ********************************************************************************/
void lwip_dump_pbuf(void *pkt, debug_rx_tx_info_t *info)
{
    struct pbuf *buf = (struct pbuf *)pkt;
	int offset = 0, len = 0;
	struct pbuf *tmp;
	unsigned char *data = buf->payload;
	
	if (!buf)
		return;
	if (!do_pkt_filter(data, buf->len, info))
		return;

	system_printf("\n---vif-%d---%s-[%d]----", info->vif,
		info->dir ? "tx" : "rx", buf->tot_len);

	offset = dump_content(buf->payload, buf->len, offset);
	for (tmp = buf->next; tmp != NULL; tmp = tmp->next) {
		offset = dump_content(tmp->payload, tmp->len, offset);
	}

	system_printf("\n-----end. len[%d]---\n", offset);
}

static int cmd_dump_pkt(cmd_tbl_t *t, int argc, char *argv[])
{
	dump_pkt_config(argc, argv);
	return CMD_RET_SUCCESS;
}

SUBCMD(debug, dump_pkt, cmd_dump_pkt, "for dump lwip pkt", "dump_pkt ");
#endif
#if 1
static inline int do_frame_filter(GenericMacHeader *gmh, debug_rx_tx_info_t *info)
{
	int match = 0;
	
	if (!dump_frame_cfg.enable)
		return match;

    if (!(dump_frame_cfg.dir & (1 << info->dir)))
        return match;
    
    if ((1 << gmh->type) & dump_frame_cfg.type && (1 << gmh->subtype) & dump_frame_cfg.stype)
        match = 1;
    
	return match;
}

void wifi_dump_frame(void *frame, debug_rx_tx_info_t *info)
{
    GenericMacHeader *gmh = (GenericMacHeader *)frame;
	
	if (!gmh)
		return;
	if (!do_frame_filter(gmh, info))
		return;
    
	system_printf("--vif%d-%s-type[%d|%d]-addr1[%02x:%02x:%02x:%02x:%02x:%02x]-"
        "addr2[%02x:%02x:%02x:%02x:%02x:%02x]---\n", info->vif,	info->dir ? "tx" : "rx", 
        gmh->type, gmh->subtype, gmh->address1[0], gmh->address1[1], gmh->address1[2], gmh->address1[3],
		gmh->address1[4], gmh->address1[5], gmh->address2[0], gmh->address2[1], gmh->address2[2], 
		gmh->address2[3], gmh->address2[4], gmh->address2[5]);
}

void dump_frame_config(int argc, char **argv)
{
	if (argc <= 1) {
		return;
	}
	
	if (argc >= 2) {
		if (!strcasecmp(argv[1], "enable")) {
			dump_frame_cfg.enable = 1;
		} else if (!strcasecmp(argv[1], "disable")) {
			dump_frame_cfg.enable = 0;
		} else if (!strcasecmp(argv[1], "show")) {
		    system_printf("enable[%d],dir[%d],type[0x%x],stype[0x%x]\n", dump_frame_cfg.enable,
                dump_frame_cfg.dir, dump_frame_cfg.type, dump_frame_cfg.stype);
			return;
		} else if (!strcasecmp(argv[1], "type") && argc == 3) {
            dump_frame_cfg.type = strtoul(argv[2], NULL, 0); 
            return;
        } else if (!strcasecmp(argv[1], "stype") && argc == 3) {
            dump_frame_cfg.stype = strtoul(argv[2], NULL, 0);
            return;
        } else if (!strcasecmp(argv[1], "dir") && argc == 3) {
            dump_frame_cfg.dir = strtoul(argv[2], NULL, 0);
            return;
        } else {
			return;
		}
		system_printf("----%s frame dump.\n", dump_frame_cfg.enable ? "enable" : "disable");
		return;
	}

    return;
}

static int cmd_dump_frame(cmd_tbl_t *t, int argc, char *argv[])
{
	dump_frame_config(argc, argv);
	return CMD_RET_SUCCESS;
}

SUBCMD(debug, dump_frame, cmd_dump_frame, "for dump 80211 frame", "dump_frame ");


#if 0//#ifdef LWIP_DEBUG
extern void lwip_debug(int argc, char **argv);
static int cmd_lwip_debug(cmd_tbl_t *t, int argc, char *argv[])
{
    lwip_debug(argc, argv);
    return CMD_RET_SUCCESS;
}

SUBCMD(debug, lwip, cmd_lwip_debug, "for debug lwip", "lwip ");
#endif

CMD(debug, NULL, "system debug cli", "debug [configure]");
#endif
