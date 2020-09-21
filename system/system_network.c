/*******************************************************************************
 * Copyright by Transa Semi.
 *
 * File Name:    
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       V1.0
 * Author:        lixiao
 * Date:          2018-12-12
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
#include "lwip/apps/lwiperf.h"
#include "apps/ping/ping.h"
#include "lwip/err.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/sys.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"
#include "system_network.h"
#include "system_wifi.h"

/****************************************************************************
* 	                                           Local Macros
****************************************************************************/
bool use_dhcp_server = true;
bool use_dhcp_client = true;
/****************************************************************************
* 	                                           Local Types
****************************************************************************/

#ifdef LWIP_PING
//static TaskHandle_t ping_task_handle = NULL;
//static TaskHandle_t iperf_task_handle = NULL;

//static struct ping_info* ping;
#endif
//char* hostname;
//bool default_hostname;
/****************************************************************************
* 	                                           Local Constants
****************************************************************************/
#ifdef LWIP_PING
const char ping_usage_str[] = "Usage: ping [-c count] [-i interval] [-s packetsize] destination\n"
                           "      `ping destination stop' for stopping\n"
                           "      `ping -st' for checking current ping applicatino status\n";
#endif
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

#ifdef LWIP_PING

u8_t get_vif_by_dstip(ip4_addr_t* addr)
{
	u8_t vif_id = STATION_IF;
	int i;
    struct netif *itf = ip_route(NULL, addr);

    if (!itf)
        return vif_id;
	for(i = 0; i < MAX_IF; i++){
		if(itf == get_netif_by_index(i)){
			vif_id = i;
			break;
		}
	}
	return vif_id;
}

//void ping_usage(void)
//{
//	system_printf( "%s\r\n", ping_usage_str);
//}

void  ping_run(char *cmd)
{
	char *str = NULL;
	char *pcmdStr_log = NULL;
	char *pcmdStr = (char *)cmd;
	double interval = PING_DELAY_DEFAULT;
	u32_t  packet_size= PING_DATA_SIZE;
	u32_t count = 0;
	ip4_addr_t ping_addr;
	u8_t vif_id = 0xFF;
	ping_parm_t* ping_conn = NULL;
	ping_parm_t* ping_stop_conn = NULL;
	int ip4addr_aton_is_valid = false;

	ip4_addr_set_any(&ping_addr);

	if(pcmdStr != NULL)
		memcpy((char **)&pcmdStr_log, (char **)&pcmdStr, sizeof((char *)&pcmdStr));

	ping_conn = (ping_parm_t *)os_zalloc(sizeof(ping_parm_t));
	if (ping_conn == NULL) {
		system_printf("memory allocation fail! [%s]\n", __func__);
		return;
	}
    ping_mutex_init();
	str = strtok_r(NULL, " ", &pcmdStr_log);
	ip4addr_aton_is_valid = ip4addr_aton(str, &ping_addr);  // Check address existence

	if(ip4addr_aton_is_valid){
		vif_id = get_vif_by_dstip(&ping_addr);
		for(;;)
		{
			str = strtok_r(NULL, " ", &pcmdStr_log);
			if(str == NULL || str[0] == '\0'){
				break;
			}else if(strcmp(str , "stop") == 0) {
				ping_stop_conn = ping_get_session(&ping_addr);
				if(ping_stop_conn!= NULL){
					ping_stop_conn->force_stop = 1;
				}else{
					system_printf("Nothing to stop : wlan%d \n", vif_id);
				}
				os_free(ping_conn);
				return;
			}else if(strcmp(str, "-i") == 0){
				str = strtok_r(NULL, " ", &pcmdStr_log);
				interval = PING_DELAY_UNIT * atof(str);
			}else if(strcmp(str, "-c") == 0){
				str = strtok_r(NULL, " ", &pcmdStr_log);
				count = atoi(str);
			}else if(strcmp(str, "-s") == 0){
				str = strtok_r(NULL, " ", &pcmdStr_log);
				packet_size = atoi(str);
			}else{
				system_printf("Error!! '%s' is unknown option. Run help ping\n", str);
				os_free(ping_conn);
				return;
			}
		}
	}else{
		if(strcmp(str, "-st") == 0){
			ping_list_display();
		}else if(strcmp(str, "-h") == 0){
			system_printf( "%s\r\n", ping_usage_str);//ping_usage();
		}else{
			system_printf("Error!! There is no target address. run hep ping\n");
		}
		os_free(ping_conn);
		return;
	}

	if(ip4_addr_isany_val(ping_addr)){
		system_printf("Error!! There is no target address. run help ping\n");
		os_free(ping_conn);
		return;
	}

	ip4_addr_copy((ping_conn->addr), ping_addr);
	ping_conn->interval = (u32_t)interval;
	ping_conn->target_count = (u32_t)count;
	ping_conn->packet_size = (u32_t)packet_size;
	ping_conn->vif_id  = vif_id;

	if(ping_get_session(&ping_conn->addr) != NULL){
		system_printf("Ping application is already running\n");
		os_free(ping_conn);
		return;
	}

	//ping_mutex_init();
	ping_conn->task_handle = (xTaskHandle)ping_init((void*)ping_conn);
	if(ping_conn->task_handle == NULL){
		os_free(ping_conn);
		return;
	}
}
#endif /* LWIP_PING */

static void set_netdb_ipconfig(int vif, struct ip_info *ipconfig)
{
    netif_db_t *netif_db = NULL;
    
    if (!IS_VALID_VIF(vif))
        return;

    netif_db = get_netdb_by_index(vif);
    netif_db->ipconfig = *ipconfig;
}

void set_softap_ipconfig(struct ip_info *ipconfig)
{
    set_netdb_ipconfig(SOFTAP_IF, ipconfig);
}

void set_sta_ipconfig(struct ip_info *ipconfig)
{
    netif_db_t *netif_db;

    netif_db = get_netdb_by_index(STATION_IF);
    netif_db->dhcp_stat = TCPIP_DHCP_STATIC;
    set_netdb_ipconfig(STATION_IF, ipconfig);
}
// ´ò¿ª dhcp
void wifi_dhcp_open(int vif)
{
	vif == SOFTAP_IF ? (use_dhcp_server = true) : (use_dhcp_client = true);
}

// ¹Ø±Õ dhcp
void wifi_dhcp_close(int vif)
{
	vif == SOFTAP_IF ? (use_dhcp_server = false) : (use_dhcp_client = false);
} 

bool get_wifi_dhcp_use_status(int vif)
{
	bool ret;

	vif == SOFTAP_IF ? (ret = use_dhcp_server) : (ret = use_dhcp_client);

	return ret;
}

int wifi_station_dhcpc_start(int vif)
{
	int8_t ret;
    struct netif *nif = NULL;
    netif_db_t *netif_db;

    if (!IS_VALID_VIF(vif))
        return false;
    
    nif = get_netif_by_index(vif);
    netif_db = get_netdb_by_index(vif);

    if (netif_db->dhcp_stat == TCPIP_DHCP_STATIC) {
        wifi_set_ip_info(vif, &netif_db->ipconfig);
        return true;
    }
    
	if(!use_dhcp_client)
		return false;

	netifapi_dhcp_release_and_stop(nif);

	ip_addr_set_zero(&nif->ip_addr);
	ip_addr_set_zero(&nif->netmask);
	ip_addr_set_zero(&nif->gw);
	netif_db->dhcp_stat = TCPIP_DHCP_STOPPED;
	

    ret = netifapi_dhcp_start(nif);
	if (!ret){
        netif_db->dhcp_stat = TCPIP_DHCP_STARTED;
	}
    SYS_LOGD("dhcpc start %s!", ret ? "fail" : "ok");
    
    return true;
}

int wifi_station_dhcpc_stop(int vif)
{
    struct netif *nif = NULL;
    netif_db_t * netif_db = NULL;

    if (!IS_VALID_VIF(vif))
        return false;
    
    nif = get_netif_by_index(vif);
    netif_db = get_netdb_by_index(vif);

    if (netif_db->dhcp_stat == TCPIP_DHCP_STATIC) {
        netif_set_addr(netif_db->net_if, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
        if (STA_STATUS_STOP == wifi_get_status(vif)) { //clear static dhcpc info
            netif_db->dhcp_stat = TCPIP_DHCP_INIT;
            memset(&netif_db->ipconfig, 0, sizeof(netif_db->ipconfig));
        }
        return true;
    }
    
	if (netif_db->dhcp_stat == TCPIP_DHCP_STARTED) {
		netifapi_dhcp_release_and_stop(nif);
        netif_db->dhcp_stat = TCPIP_DHCP_STOPPED;
    	SYS_LOGV("stop dhcp client");
	}
    
	return true;
}

/*get dhcpc/dhcpserver status*/
dhcp_status_t wifi_station_dhcpc_status(int vif)
{
    netif_db_t * netif_db = NULL;

    if (!IS_VALID_VIF(vif))
        return TCPIP_DHCP_STATUS_MAX;
    
    netif_db = get_netdb_by_index(vif);
    return netif_db->dhcp_stat;
}

void wifi_softap_dhcps_start(int intf)
{
    struct ip_info ipinfo_tmp;
    netif_db_t *netif_db;

    if (!IS_VALID_VIF(intf))
        return;

    netif_db = get_netdb_by_index(intf);
    if (netif_db->dhcp_stat == TCPIP_DHCP_STARTED) {
        SYS_LOGD("start dhcps, dhcps already started.");
        return;
    }

    if (ip4_addr_isany(&netif_db->ipconfig.ip)) {
    	IP4_ADDR(&netif_db->ipconfig.ip, 10, 10, 10, 1);
    	IP4_ADDR(&netif_db->ipconfig.gw, 10, 10, 10, 1);
    	IP4_ADDR(&netif_db->ipconfig.netmask, 255, 255, 255, 0);
    }
#ifdef ENABLE_LWIP_NAPT
    if (ip4_addr_isany(&netif_db->ipconfig.dns1)) {
        netif_db->ipconfig.dns1 = (IP_ADDR_ANY != dns_getserver(0)) ? *dns_getserver(0) : netif_db->ipconfig.ip;
        netif_db->ipconfig.dns2 = (IP_ADDR_ANY != dns_getserver(1)) ? *dns_getserver(1) : netif_db->ipconfig.ip;
    }
#endif      
	wifi_set_ip_info(intf, &netif_db->ipconfig);
	if(!use_dhcp_server)
		return;
	dhcps_start(get_netif_by_index(intf), &netif_db->ipconfig);
    netif_db->dhcp_stat = TCPIP_DHCP_STARTED;
}

void wifi_softap_dhcps_stop(int intf)
{
    struct ip_info ipinfo_tmp;
    netif_db_t *netif_db;

    if (!IS_VALID_VIF(intf))
        return;

    netif_db = get_netdb_by_index(intf);
    if (netif_db->dhcp_stat != TCPIP_DHCP_STARTED) {
        SYS_LOGD("stop dhcps, but dhcps not started.");
        return;
    }

    memset(&ipinfo_tmp, 0, sizeof(ipinfo_tmp));
    wifi_set_ip_info(intf, &ipinfo_tmp);
    dhcps_stop();
    netif_db->dhcp_stat = TCPIP_DHCP_STOPPED;
}

dhcp_status_t wifi_softap_dhcps_status(int vif)
{
    netif_db_t * netif_db = NULL;
    dhcp_status_t ret = TCPIP_DHCP_INIT;

    if (!IS_VALID_VIF(vif))
        return TCPIP_DHCP_STATUS_MAX;

    netif_db = get_netdb_by_index(vif);
    ret = netif_db->dhcp_stat;
    
    return ret;
}

void wifi_ifconfig_help_display(void)
{
	system_printf("Usage:\n");
	system_printf("   ifconfig <interface> [<address>]\n");
	system_printf("   ifconfig <interface> [mtu <NN>]\n");
}

void wifi_ifconfig_display(wifi_interface_e if_index)
{
	struct netif *netif_temp = get_netif_by_index(if_index);

    if (!netif_is_up(netif_temp))
        return;
	system_printf("wlan%d     ", if_index);
	system_printf("HWaddr ");
	system_printf(MAC_STR,MAC_VALUE(netif_temp->hwaddr) );
	system_printf("   MTU:%d\n", netif_temp->mtu);
	system_printf("          inet addr:");
	system_printf(IP4_ADDR_STR,IP4_ADDR_VALUE(&(netif_temp->ip_addr)) );
	system_printf("   Mask:");
	system_printf(IP4_ADDR_STR,IP4_ADDR_VALUE(&(netif_temp->netmask)) );
	system_printf("   Gw:");
	system_printf(IP4_ADDR_STR,IP4_ADDR_VALUE(&(netif_temp->gw)) );
	system_printf("\n");
}

void wifi_ifconfig(char* cmd)
{
	char *str = NULL;
	char *pcmdStr_log = NULL;
	char *pcmdStr = (char *)cmd;
	struct ip_info info;
	struct netif *netif_temp;
	int i;

	if(pcmdStr != NULL)
		memcpy((char **)&pcmdStr_log, (char **)&pcmdStr, sizeof((char *)&pcmdStr));

	str = strtok_r(NULL, " ", &pcmdStr_log);
	if(str == NULL || str[0] == '\0'){
		wifi_ifconfig_display(STATION_IF);
		wifi_ifconfig_display(SOFTAP_IF);
	} else {
		 if(strcmp(str, "wlan0") == 0){
			netif_temp = get_netif_by_index(STATION_IF);
			i = STATION_IF;
		}else if(strcmp(str, "wlan1") == 0){
			netif_temp = get_netif_by_index(SOFTAP_IF);
			i = SOFTAP_IF;
		}else if(strcmp(str, "-h") == 0){
			wifi_ifconfig_help_display();
			return;
		}else{
			system_printf("%s is unsupported. run ifconfig -h\n", str);
			return;
		}

		str = strtok_r(NULL, " ", &pcmdStr_log);
		if(str == NULL || str[0] == '\0'){
			wifi_ifconfig_display(i);
		}else if(strcmp(str, "mtu") == 0){
			str = strtok_r(NULL, " ", &pcmdStr_log);
			netif_temp->mtu = atoi(str);
		}else{
			ip4addr_aton(str, &info.ip);
			IP4_ADDR(&info.gw, ip4_addr1_16(&info.ip), ip4_addr2_16(&info.ip), ip4_addr3_16(&info.ip), 1);
			IP4_ADDR(&info.netmask, 255, 255, 255, 0);
			netif_set_addr(netif_temp, &info.ip, &info.netmask, &info.gw);
		}
	}
}

bool wifi_set_ip_info(wifi_interface_e if_index, ip_info_t *info)
{
	netif_set_addr(get_netif_by_index(if_index), &info->ip,  &info->netmask, &info->gw);
    if (!ip4_addr_isany(&info->dns1) || !ip4_addr_isany(&info->dns1)) {
        dns_setserver(0, &info->dns1);
        dns_setserver(1, &info->dns2);
    }
	return true;
}

bool wifi_get_ip_info(wifi_interface_e if_index, struct ip_info *info)
{
    struct netif *nif = NULL;
    
    nif = get_netif_by_index(if_index);
	info->ip = nif->ip_addr;
	info->netmask = nif->netmask;
	info->gw = nif->gw;
    info->dns1 = *dns_getserver(0);
    info->dns1 = *dns_getserver(1);
    
	return true;
}


