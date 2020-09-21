#include "ya_common.h"
#include "smnt_pub.h"
#include "wifi_80211.h"
#include "ya_smartconfig.h"
#include "ya_common_func.h"
#include "ya_aes_md5.h"

//#define SMART_DEBUG

uint32_t channel_lock_time = 0;

ya_hal_os_thread_t smart_config_task;
ROUTER_INFO *info_head = NULL;
SCONFIG_INFO *sconfig_info = NULL;

smnt_param_t smartconfig_gobal;
smnt_result_t smnt_result;

static uint8_t run_smartconfig=0;

static const uint8_t broadcast_mac[6]={0xff,0xff,0xff,0xff,0xff,0xff};
static const uint8_t multicast_mac[3]={0x01,0x00,0x5e};
static const uint8_t channel_list[13]={1,6,11,4,8,13,2,5,9,3,7,12,10};

static const uint8_t len_fixed_offset[ENC_TYPE_MAX + 1][2] = {
    {  /* open, none, ip(20) + udp(8) + 8(LLC) */
        36, 36
    },
    {  /* wep, + iv(4) + data + ICV(4) */
        44, 44  /* feixun, wep64(10byte), wep128(26byte) */
    },
    {  /* tkip, + iv/keyID(4) + Ext IV(4) + data + MIC(8) + ICV(4) */
        56, 56  /* tkip(10byte, 20byte), wpa2+tkip(20byte) */
    },
    {  /* aes, + ccmp header(8) + data + MIC(8) + ICV(4) */
        52, 52
    },
    {  /* tkip-aes */
        56, 52  /* fromDs==tkip,toDs==aes */
    }
};

static void sconfig_init(void)
{
    info_head = (ROUTER_INFO *)ya_hal_os_memory_alloc(sizeof(ROUTER_INFO));
    info_head->next = NULL;

    sconfig_info = (SCONFIG_INFO *)ya_hal_os_memory_alloc(sizeof(SCONFIG_INFO));
    sconfig_info->switchChannel = CHANNEL_SEARCH_START;
    sconfig_info->findPhone = NOT_ACTIVE;
    sconfig_info->delay = 60;
}

static int add_node_info(ROUTER_INFO *info)
{
    ROUTER_INFO *head = info_head;
    int cnt = 0;
    while(head->next){
        head = head->next;  // 指向下一个
        cnt++;
    }
    head->next = info;  // 将info加到链表尾部
    cnt++;
    return cnt;
}

static void free_node_info(void)
{
    ROUTER_INFO *head = info_head;
    while(head->next){
        ya_hal_os_memory_free(head->next);
        head = head->next;  // 指向下一个
    }
    info_head->next = NULL;  
}

static int set_channel( int ch)
{
	return ya_hal_wlan_set_channel(ch);
}

static ROUTER_INFO  *get_ap_info(uint8_t *bssid)
{
	struct router_info *info  = info_head; // router 链表head
	while (info != NULL){
		if (0 == memcmp((uint8_t *)info->bssid, (uint8_t *)bssid, 6)){
			return info;
		}
		info = info->next;
	}
	return NULL;
  
}

static int get_ap_info_channel(uint8_t *bssid)
{
	struct router_info *info  = info_head; // router 链表head
	while (info != NULL){
		if (0 == memcmp((uint8_t *)info->bssid, (uint8_t *)bssid, 6)){
			return info->channel;
		}
		info = info->next;
	}
	return 0;
}


static int get_ap_info_key(uint8_t *bssid)
{
	struct router_info *info  = info_head; // router 链表head
	while (info != NULL){
		if (0 == memcmp((uint8_t *)info->bssid, (uint8_t *)bssid, 6)){
			if	(AUTH_TYPE_OPEN == info->auth)
				return 1;
		}
		info = info->next;
	}
	return 0;
}

static  char *auth_privacy_string(uint8_t auth)
{
    char *sec;

    switch (auth)
    {
    case AUTH_TYPE_OPEN:
        sec = "OPEN";
        break;
    case AUTH_TYPE_SHARED:
        sec = "SHARED";
        break;
    case AUTH_TYPE_WPAPSK:
        sec = "WPAPSK";
        break;
    case AUTH_TYPE_WPA8021X:
        sec = "WPA8021X";
        break;
    case AUTH_TYPE_WPA2PSK:
        sec = "WPA2PSK";
        break;
    case AUTH_TYPE_WPA28021X:
        sec = "WPA28021X";
        break;
    case AUTH_TYPE_WPAPSKWPA2PSK:
        sec = "WPAPSKWPA2PSK";
        break;
	case AUTH_TYPE_INVALID:
        sec = "INVALID";
        break;	
    default:
        sec = "Unknown";
        break;
    }
    return sec;
}

static  char *cipher_privacy_string(uint8_t cipher)
{
    char *sec;
    switch (cipher)
    {
    case ENC_TYPE_NONE:
        sec = "";
        break;
    case ENC_TYPE_WEP:
        sec = "WEP";
        break;
    case ENC_TYPE_TKIP:
        sec = "TKIP";
        break;
    case ENC_TYPE_AES:
        sec = "AES";
        break;
    case ENC_TYPE_TKIPAES:
        sec = "TKIPAES";
        break;
	case ENC_TYPE_INVALID:
        sec = "INVALID";
        break;	
    default:
        sec = "Unknown";
        break;
    }
    return sec;
}

static int save_aplist (uint8_t *ssid, uint8_t *bssid, uint8_t channel, uint8_t auth,
                     uint8_t pairwise_cipher, uint8_t group_cipher )
{
    /* ssid, bssid cannot empty, channel can be 0, auth/encry can be invalid */
    if (!(ssid && bssid)) {
        return -1;
    }

    if (auth > AUTH_TYPE_MAX) {
        auth = AUTH_TYPE_INVALID;
    }

    if (pairwise_cipher > ENC_TYPE_MAX) {
        pairwise_cipher = ENC_TYPE_INVALID;
    }
    if (group_cipher > ENC_TYPE_MAX) {
        group_cipher = ENC_TYPE_INVALID;
    }

    /* FIXME: */
    if (pairwise_cipher == ENC_TYPE_TKIPAES) {
        pairwise_cipher = ENC_TYPE_AES;    /* tods */
    }
    if(NULL !=get_ap_info(bssid)){
		return -1;
    }

    ROUTER_INFO *info = (ROUTER_INFO *)ya_hal_os_memory_alloc(sizeof(ROUTER_INFO));

	memset((char *)info->ssid, 0, MAX_SSID_LEN);
    strncpy((char *)info->ssid, (char const *)ssid, MAX_SSID_LEN - 1);
    
    memcpy(info->bssid, bssid, 6); 
    info->auth = auth;         
    info->channel = channel;       
    info->encry[0] = group_cipher;
    info->encry[1] = pairwise_cipher;
    info->next = NULL;
    //sconfig_info->channel_bits |= (1 << channel);

    printf("%s\t",ssid);
    printf("\t%02x%02x%02x%02x%02x%02x\t", bssid[0], bssid[1], bssid[2],
               bssid[3], bssid[4], bssid[5]);
    printf("\t%d\t", channel);
    printf("%s\t", auth_privacy_string(auth));
    printf("%s ", cipher_privacy_string(group_cipher));
    printf("%s\r\n", cipher_privacy_string(pairwise_cipher));
    
    add_node_info(info);
    return 0;
}

static void smartconfig_finish(int result_status)
{
	if(smartconfig_gobal.get_result_callback != NULL)
	{
		smnt_result.smnt_result_status = result_status;
		smartconfig_gobal.get_result_callback(&smnt_result);	
	}
}

typedef struct 
{
	uint8_t  key[80]; 
	uint8_t  key_flag[80];
	uint8_t  key_len;
	uint8_t  key_cur_index;
	uint8_t  key_checksum;
	uint8_t key_checksum_flag;
	uint8_t count_num;
	uint8_t finish;
	uint8_t status;
}ya_parse_para_broadcast_t;

typedef struct 
{
	uint8_t  key[128]; 
	uint8_t  key_flag[128];
	uint8_t  key_len;
	uint8_t  key_cur_index;
	uint8_t  key_checksum;
	uint8_t key_checksum_flag;
	uint8_t count_num;
	uint8_t finish;
}ya_parse_para_multicast_t;


ya_parse_para_multicast_t *ya_parse_mul = NULL;
ya_parse_para_broadcast_t *ya_parse_to_ds = NULL;
ya_parse_para_broadcast_t *ya_parse_from_ds = NULL;

void ya_parse_multicast(uint8_t *bssid,uint8_t *ds_mac,uint8_t *sa_mac, int len, ya_parse_para_multicast_t *ya_parse_m)
{
	uint8_t index = 0, tmp = 0;
	uint8_t check_cacul = 0;

	uint16_t tmp_len_check = 0;

    if(sconfig_info->switchChannel == CHANNEL_SEARCH_END) 	// 信道已确定
    {
	    if( (0 != memcmp(sa_mac, sconfig_info->srcmac, 6) ) || (0 != memcmp(sconfig_info->bssid, bssid, 6) ) )
			return;
    }

	ds_mac[0] = (ds_mac[0]&0x7F);

	//ya_printf(C_LOG_INFO, "%02x:%02x:%02x\r\n", ds_mac[0], ds_mac[1], ds_mac[2]);

	tmp = ds_mac[0] + ds_mac[1] + ds_mac[2];
	tmp_len_check = tmp  + 1;

	//ya_printf(C_LOG_INFO, "%d, %d\r\n", len, tmp_len_check);

	if (sconfig_info->switchChannel == CHANNEL_SEARCH_START)
	{
		if (len == tmp_len_check)
			ya_parse_m->count_num++;
		else
			ya_parse_m->count_num = 0;

		if (ya_parse_m->count_num > 1)
		{
			sconfig_info->switchChannel = CHANNEL_SEARCH_END;
			memcpy(sconfig_info->srcmac, sa_mac, 6);
			memcpy(sconfig_info->bssid,bssid,6);
			ya_printf(C_LOG_INFO,"multicast lock!!!\r\n");
			ya_printf(C_LOG_INFO,"sa mac=("MAC_FMT") bssid=("MAC_FMT")\r\n", MAC_ARG(sa_mac),MAC_ARG(bssid));
		}

	}else
	{
		if (len != tmp_len_check)
			return;
	
		if (ds_mac[0] == 0x20 && (ds_mac[1] == ds_mac[2]))
		{
			channel_lock_time = ya_hal_os_ticks_to_msec();
			
			if (ds_mac[1] >= 8 && ds_mac[1] <= 128)
				ya_parse_m->key_len = ds_mac[1];
			else
				return;
		}else if(ds_mac[0] & 0x40)
		{
			channel_lock_time = ya_hal_os_ticks_to_msec();
		
			tmp = ds_mac[0] & 0x3F;
			ya_parse_m->key[tmp*2] = ds_mac[1];
			ya_parse_m->key[tmp*2 + 1] = ds_mac[2];

			ya_parse_m->key_flag[tmp*2] = 1;
			ya_parse_m->key_flag[tmp*2 + 1] = 1;
		}else if(ds_mac[0] == 0x01 && (ds_mac[1] == ds_mac[2]))
		{
			channel_lock_time = ya_hal_os_ticks_to_msec();

			ya_parse_m->key_checksum = ds_mac[1];
			ya_parse_m->key_checksum_flag = 1;
		}else
		{
			return;
		}

		if (ya_parse_m->key_checksum_flag && ya_parse_m->key_len)
		{
			for (index = 0; index < ya_parse_m->key_len; index++)
			{
				if (ya_parse_m->key_flag[index] == 0)
					break;

				check_cacul += ya_parse_m->key[index];
			}

			if (index == ya_parse_m->key_len)
			{
				if (ya_parse_m->key_checksum == check_cacul)
				{
					ya_printf(C_LOG_INFO,"=======finish multicast=======\r\n");
					sconfig_info->findPhone = OK_KEY;
					ya_parse_m->finish = 1;
				}else
				{
					ya_printf(C_LOG_INFO,"checksum error, %d, %d\r\n", check_cacul, ya_parse_m->key_checksum);
				}
			}			
		}
	}
	
}

static void ya_parse_broadcast(uint8_t *bssid, uint8_t *sa_mac, int len, ya_parse_para_broadcast_t *ya_parse_b, uint8_t flag)
{	
	uint8_t index = 0;
	uint8_t check_cacul = 0;

	if (sconfig_info->findPhone == OK_KEY)
		return;
	
	if (sconfig_info->switchChannel == CHANNEL_SEARCH_END)
	{		 
		if ((0 != memcmp(sa_mac, sconfig_info->srcmac, 6) ) || (0 != memcmp(sconfig_info->bssid, bssid, 6) ) )
			return ;
	}

	#ifdef SMART_DEBUG
	if (flag == 0)
		ya_printf(C_LOG_INFO, "t-%d\r\n", len);
	else
		ya_printf(C_LOG_INFO, "f-%d\r\n", len);
	#endif

	if (sconfig_info->switchChannel == CHANNEL_SEARCH_START)
	{
		if (len == 3)
		{
			ya_parse_b->count_num++;
			if (ya_parse_b->count_num > 3)
			{
				sconfig_info->switchChannel = CHANNEL_SEARCH_END;
				ya_printf(C_LOG_INFO,"broadcast lock!!!\r\n");
				ya_printf(C_LOG_INFO,"sa mac=("MAC_FMT") bssid=("MAC_FMT")\r\n", MAC_ARG(sa_mac),MAC_ARG(bssid));

				memcpy(sconfig_info->srcmac, sa_mac, 6);
				memcpy(sconfig_info->bssid, bssid,6);

				ya_parse_b->status = SCONFIG_INDEX;
				memset(ya_parse_b, 0, sizeof(ya_parse_para_broadcast_t));
			}
		} else
		{
			ya_parse_b->count_num = 0;
		}

	}else 
	{
		if (len == 1)
		{
			channel_lock_time = ya_hal_os_ticks_to_msec();
			ya_parse_b->status = SCONFIG_LEN;
			ya_parse_b->key_cur_index = 0xff;
		}else if (len == 3)
		{
			channel_lock_time = ya_hal_os_ticks_to_msec();
			ya_parse_b->status = SCONFIG_INDEX;
			ya_parse_b->key_cur_index = 0xff;
		}else if (len == 5)
		{
			channel_lock_time = ya_hal_os_ticks_to_msec();
			ya_parse_b->status = SCONFIG_CHK;
			ya_parse_b->key_cur_index = 0xff;
		}else 
		{
			switch (ya_parse_b->status)
			{
				case SCONFIG_LEN:
					if (len < 6 || len > 80)
						return;

					ya_parse_b->key_len = len;
					ya_parse_b->key_cur_index = 0xff;
					ya_parse_b->status = SCONFIG_IDLE;
				break;

				case SCONFIG_INDEX:
					if (len < 7 || len > 87)
						return;

					ya_parse_b->key_cur_index = len - 7;
					ya_parse_b->status = SCONFIG_KEY;

				break;
				case SCONFIG_KEY:
					if (len < 100 || len > (255 + 100))
						return;	

					ya_parse_b->key[ya_parse_b->key_cur_index] = len - 100;
					ya_parse_b->key_flag[ya_parse_b->key_cur_index] = 1;
					ya_parse_b->status = SCONFIG_IDLE;
				break;

				case SCONFIG_CHK:
					if (len < 100 || len > (255 + 100))
						return; 

					ya_parse_b->key_checksum = len - 100;
					ya_parse_b->key_checksum_flag = 1;
					ya_parse_b->status = SCONFIG_IDLE;
					break;
				default:
					break;
			}

			if (ya_parse_b->key_checksum_flag && ya_parse_b->key_len)
			{
				for (index = 0; index < ya_parse_b->key_len; index++)
				{
					if (ya_parse_b->key_flag[index] == 0)
						break;

					check_cacul += ya_parse_b->key[index];
				}

				if (index == ya_parse_b->key_len)
				{
					if (ya_parse_b->key_checksum == check_cacul)
					{
						ya_parse_b->finish = 1;
						sconfig_info->findPhone = OK_KEY;
						ya_printf(C_LOG_INFO,"=======finish broadcast=======: %d\r\n", ya_parse_b->key_len);
					}else
					{
						ya_printf(C_LOG_INFO,"checksum error, %d, %d\r\n", check_cacul, ya_parse_b->key_checksum);
					}
				}
			}
		}

	}
	
}


static int wifi_80211_aplist_process(uint8_t *mgmt_header, int len)
{
    uint8_t ssid[32+1] = {0}, bssid[6] = {0};
    uint8_t auth, pairwise_cipher, group_cipher;
    int ret, channel;

    if (mgmt_header == NULL) {
        return 0;
    }

    ret = wifi_80211_get_bssid(mgmt_header, bssid);
    if (ret < 0) {
        return 0;
    }

    ret = wifi_80211_get_ssid(mgmt_header, len, ssid);
    if (ret < 0) {
        return 0;
    }

    channel = wifi_80211_get_bss_channel(mgmt_header, len);

    wifi_80211_get_cipher_info(mgmt_header, len, &auth,
                             &pairwise_cipher, &group_cipher);
	
	save_aplist(ssid, bssid, channel, auth,
                     pairwise_cipher, group_cipher);
    return 0;
}


/* convert IEEE 802.11 header + possible LLC headers into Ethernet header
* IEEE 802.11 address fields:
* ToDS FromDS Addr1 Addr2 Addr3 Addr4
*   0     0   DA    SA    BSSID n/a
*   0     1   DA    BSSID SA    n/a
*   1     0   BSSID SA    DA    n/a
*   1     1   RA    TA    DA    SA
*/
static void easyjoin_callback(uint8_t* userdata, int data_len)
{
	struct wifi_80211_hdr *hdr = (struct wifi_80211_hdr*)userdata;
	uint8_t *data, *bssid_mac, *da_mac, *sa_mac;
	uint8_t tods = 0, fromds = 0, encry = ENC_TYPE_INVALID;;
	ROUTER_INFO *ap_info;
	int hdrlen ,fc,len;

	fc = hdr->frame_control;

	/* drop frag, more, order*/
	if (wifi_80211_has_frags(fc)) {
		return ;
	}

	if ( wifi_80211_is_beacon(fc) || wifi_80211_is_probe_resp(fc)) {
		/*bracon / Probe Response*/
		wifi_80211_aplist_process(userdata,data_len); 
	}

	if(sconfig_info->findPhone != NOT_ACTIVE){
		return;
	}

	/*
	* for smartconfig with bcast of data
	*/
	if (!wifi_80211_is_data_exact(fc)) {
		return ;
	}

	/*
	* payload len = frame.len - (radio_header + wlan_hdr)
	*/
	hdrlen = wifi_80211_hdrlen(fc);
	if (hdrlen > data_len) {
		return ;
	}

	data = userdata + hdrlen;  
	len = data_len -hdrlen;

	tods = wifi_80211_has_tods(fc);
	fromds = wifi_80211_has_fromds(fc);
	bssid_mac = (uint8_t *)wifi_80211_get_BSSID(hdr);
	da_mac = (uint8_t *)wifi_80211_get_DA(hdr);
	sa_mac = (uint8_t *)wifi_80211_get_SA(hdr);

	ap_info=get_ap_info(bssid_mac);

	if (ap_info && ENC_TYPE_INVALID != ap_info->encry[tods]) {
		encry = ap_info->encry[tods];
	} 
	else
	{
		if (!wifi_80211_has_protected(fc)) {
				encry = ENC_TYPE_NONE;
		} else {
			/* Note: avoid empty null data */
			if (len < 8) {      /* IV + ICV + DATA >= 8 */
				return ;
			}
			if (!(userdata[3] & 0x3F)) {
					encry = ENC_TYPE_WEP;//len=len-68;
			} else if (data[3] & (1 << 5)) {/* Extended IV */
				if (data[1] == ((data[0] | 0x20) & 0x7F)) { /* tkip, WEPSeed  = (TSC1 | 0x20 ) & 0x7F */
					encry = ENC_TYPE_TKIP;
				}
				if (data[2] == 0 && (!(data[3] & 0x0F))) {
					encry = ENC_TYPE_AES;
				}
			}
		}
	}

	len -= len_fixed_offset[encry][0];    /* dont't care about tkip-aes */

	if (wifi_80211_is_retry(fc))
		return;

	if (!memcmp(da_mac, broadcast_mac, 6)) 
	{
	#if 1
		if (tods == 1 )
			ya_parse_broadcast(bssid_mac, sa_mac, len, ya_parse_to_ds, 0);
		else if (fromds == 1 )
			ya_parse_broadcast(bssid_mac, sa_mac, len, ya_parse_from_ds, 1);
	#endif	
    }
	else if (!memcmp(da_mac, multicast_mac, 3)){
		#if 1
		ya_parse_multicast(bssid_mac, da_mac+3, sa_mac, len, ya_parse_mul);
		#endif
	}
}

int ya_smartconfig_decode(void)
{
	uint8_t *buf = NULL, *p = NULL;
	uint16_t in_len = 0, out_len = 0;

	uint8_t tmp[72] = {0};
	uint8_t aes[16] = {0};
	uint8_t iv[16] = {0};
	uint8_t index = 0, calu = 0, pos = 0;
	uint8_t randA = 0, randB = 0;

	ROUTER_INFO *ap_info;

	memset(aes, 0, sizeof(aes));
	ya_mbedtls_md5(AES_KEY_SMARTCONFIG_STRING, aes, strlen(AES_KEY_SMARTCONFIG_STRING));
	
	memcpy(iv, aes, 16); 

	#ifdef SMART_DEBUG
	ya_printf(C_LOG_INFO,"aes is: \r\n");
	for (index = 0; index < 16; index++)
	{
		ya_printf(C_LOG_INFO," %02x", aes[index]);
	}

	ya_printf(C_LOG_INFO,"\r\n");
	#endif


	buf = (uint8_t *)ya_hal_os_memory_alloc(128);
	if (!buf) return -1;
	
	memset(buf, 0, 128);

	if (ya_parse_to_ds->finish == 1)
	{
		p = ya_parse_to_ds->key;
		in_len = ya_parse_to_ds->key_len;
	}else if (ya_parse_from_ds->finish == 1)
	{
		p = ya_parse_from_ds->key;
		in_len = ya_parse_from_ds->key_len;
	}else if (ya_parse_mul->finish == 1)
	{
		p = ya_parse_mul->key;
		in_len = ya_parse_mul->key_len;
	}else
	{
		goto err;
	}

	if (in_len == 0)
		goto err;
	
	out_len = AES_CFB_NoPadding_decrypt(aes,iv, p, buf, in_len);

	#ifdef SMART_DEBUG
	ya_printf(C_LOG_INFO,"in data: %d\r\n", out_len);
	for (index = 0; index < in_len; index++ )
	{
		ya_printf(C_LOG_INFO," %02x", p[index]);
	}
	#endif
	
	ya_printf(C_LOG_INFO,"\r\n out data: \r\n");
	for (index = 0; index < out_len; index++ )
	{
		ya_printf(C_LOG_INFO," %02x", buf[index]);
	}
	
	ya_printf(C_LOG_INFO,"\r\n");

	for (index = 0; index < (out_len - 1); index++ )
		calu += buf[index];


	if(calu == buf[index])
	{
		ya_printf(C_LOG_INFO, "decode ok\r\n");
	}else
	{
		goto err;
	}

	ap_info = get_ap_info(sconfig_info->bssid);

	memset(&smnt_result, 0, sizeof(smnt_result_t));
	if (ya_parse_mul->finish == 1)
	{	
		if (buf[0] > 32 || buf[0] == 0)
			goto err;
		else
			smnt_result.ssid_len = buf[0];

		if (out_len < (1 + buf[0] + 1))
			goto err;

		memcpy(smnt_result.ssid, buf+1, smnt_result.ssid_len);
		pos = 1 + smnt_result.ssid_len;

		if (out_len < (1 + pos + buf[pos] + 1))
			goto err;

		if (buf[pos] > 64)
			goto err;

		smnt_result.password_len = buf[pos];
		memcpy(smnt_result.password, buf + pos + 1, smnt_result.password_len);

		pos += 1 + smnt_result.password_len;
	
		if (out_len < (1 + pos + buf[pos] + 1))
			goto err;

		if (buf[pos] < 3)
			goto err;

		pos++;
		smnt_result.cloud_select = buf[pos++];
		randA = buf[pos++];
		randB = buf[pos++];

	}else 
	{	
		pos = 0;

		if (buf[pos] > 64)
			goto err;

		smnt_result.password_len = buf[pos];
		memcpy(smnt_result.password, buf + pos + 1, smnt_result.password_len);

		pos += 1 + smnt_result.password_len;
	
		if (out_len < (1 + pos + buf[pos] + 1))
			goto err;

		if (buf[pos] < 3)
			goto err;

		pos++;
		smnt_result.cloud_select = buf[pos++];
		randA = buf[pos++];
		randB = buf[pos++];
	}

	memcpy(smnt_result.bssid, sconfig_info->bssid, 6);
	if (ap_info != NULL)
	{
		if(ap_info->ssid[0] != 0)
		{
			memset(smnt_result.ssid, 0, 33);
			strcpy(smnt_result.ssid, ap_info->ssid);
			smnt_result.ssid_len = strlen(ap_info->ssid);
		}
	}else
	{
		if (smnt_result.ssid[0] == 0)
			goto err;
	}

	memset(tmp, 0, 72);
	strcpy(tmp, smnt_result.password);
	HexArrayToString(&randA, 1, tmp + smnt_result.password_len);
	HexArrayToString(&randB, 1, tmp + smnt_result.password_len + 2);

	#ifdef SMART_DEBUG
	ya_printf(C_LOG_INFO,"before md5: %s\r\n", tmp);
	#endif

	ya_mbedtls_md5((uint8_t *)tmp, aes, strlen(tmp));
	HexArrayToString(aes, 16, smnt_result.rand);

	#ifdef SMART_DEBUG
	ya_printf(C_LOG_INFO,"after md5: %s\r\n", smnt_result.rand);
	#endif
	
	ya_printf(C_LOG_INFO,"ssid: %d, %s, pwd: %d, %s\r\n", smnt_result.ssid_len, smnt_result.ssid, smnt_result.password_len, smnt_result.password);	
	ya_hal_os_memory_free(buf);
	return 0;

	err:
	if(buf)
		ya_hal_os_memory_free(buf);

	return -1;
}

void smartconfig_task(void *param)
{
	uint32_t start_time = 0, first_time = 0, current_time = 0;    
	uint8_t fix_channel_flag = 0, fix_channel = 0;
	int channel = 0;		
	int ret = 0;

	ya_wifi_start_monitor(easyjoin_callback);

	start_time = ya_hal_os_ticks_to_msec();
	first_time = ya_hal_os_ticks_to_msec();
	channel_lock_time = ya_hal_os_ticks_to_msec();

	memset(ya_parse_to_ds, 0, sizeof(ya_parse_para_broadcast_t));
	memset(ya_parse_from_ds, 0, sizeof(ya_parse_para_broadcast_t));
	memset(ya_parse_mul, 0, sizeof(ya_parse_para_multicast_t));
	
	while(1)
	{
		ya_delay(10);   
		current_time = ya_hal_os_ticks_to_msec();
		if( (current_time - first_time) > (smartconfig_gobal.timeout*1000) ){
			ya_wifi_stop_monitor();
			smartconfig_finish(smnt_result_overtime);
			goto ejoin_out;
		}

		if (OK_KEY == sconfig_info->findPhone) 
		{ 
			ret = ya_smartconfig_decode();

			if (ret < 0)
			{
				ya_printf(C_LOG_INFO, "decode fail\r\n");
				memset(ya_parse_to_ds, 0, sizeof(ya_parse_para_broadcast_t));
				memset(ya_parse_from_ds, 0, sizeof(ya_parse_para_broadcast_t));
				memset(ya_parse_mul, 0, sizeof(ya_parse_para_multicast_t));
				sconfig_info->switchChannel = CHANNEL_SEARCH_START;
				sconfig_info->findPhone = NOT_ACTIVE;
				fix_channel_flag = 0;
			}else
			{
				ya_wifi_stop_monitor();
				smartconfig_finish(smnt_result_ok);
				goto ejoin_out;
			}			
		}else {
			if (sconfig_info->switchChannel == CHANNEL_SEARCH_START)
			{
				channel_lock_time = ya_hal_os_ticks_to_msec();
			
				if( (current_time - start_time) >= sconfig_info->delay )
				{
					if(channel > 12) 
						channel = 0;
					set_channel(channel_list[channel]);
					channel++;
					start_time = ya_hal_os_ticks_to_msec(); 

					ya_printf(C_LOG_INFO,"c: %d\n", channel);
				}
			} else
			{
				if( (current_time - channel_lock_time) > (8*1000) )
				{
					channel_lock_time = ya_hal_os_ticks_to_msec();

					memset(ya_parse_to_ds, 0, sizeof(ya_parse_para_broadcast_t));
					memset(ya_parse_from_ds, 0, sizeof(ya_parse_para_broadcast_t));
					memset(ya_parse_mul, 0, sizeof(ya_parse_para_multicast_t));
					sconfig_info->switchChannel = CHANNEL_SEARCH_START;
					sconfig_info->findPhone = NOT_ACTIVE;
					fix_channel_flag = 0;
				}

			}
		}

		if(fix_channel_flag == 0 && sconfig_info->switchChannel == CHANNEL_SEARCH_END)
		{
			fix_channel = get_ap_info_channel(sconfig_info->bssid);
			if(fix_channel != 0)
			{
				fix_channel_flag = 1;
				ya_printf(C_LOG_INFO,"\r\n fix channel: %d\n",fix_channel);
				set_channel(fix_channel);
			}
		}
	}
ejoin_out:
	run_smartconfig = 0;
	free_node_info();

	if(sconfig_info != NULL)
		ya_hal_os_memory_free(sconfig_info);

	if(ya_parse_to_ds != NULL)
		ya_hal_os_memory_free(ya_parse_to_ds);

	if(ya_parse_from_ds != NULL)
		ya_hal_os_memory_free(ya_parse_from_ds);

	if(ya_parse_mul != NULL)
		ya_hal_os_memory_free(ya_parse_mul);

	info_head = NULL;
	sconfig_info = NULL;
	smartconfig_gobal.get_result_callback=NULL;
	ya_hal_os_thread_delete(NULL);  
}


int start_smnt(smnt_param_t param)
{
	ya_printf(C_LOG_INFO,"%s|%d----------\r\n",__FUNCTION__,__LINE__);

	if(run_smartconfig)
		return -1;

	run_smartconfig = 1;

	if(param.get_result_callback != NULL){
		smartconfig_gobal.get_result_callback =param.get_result_callback;
	}

	smartconfig_gobal.timeout = param.timeout;

	if(!ya_parse_to_ds)
		ya_parse_to_ds = (ya_parse_para_broadcast_t *)ya_hal_os_memory_alloc(sizeof(ya_parse_para_broadcast_t));

	if(!ya_parse_from_ds)
		ya_parse_from_ds = (ya_parse_para_broadcast_t *)ya_hal_os_memory_alloc(sizeof(ya_parse_para_broadcast_t));

	if(!ya_parse_mul)
		ya_parse_mul = (ya_parse_para_multicast_t *)ya_hal_os_memory_alloc(sizeof(ya_parse_para_multicast_t));

	sconfig_init();
 
	if(ya_hal_os_thread_create(&smart_config_task, "smartconfig", smartconfig_task, 0, (1024*2), 7) != C_OK)
	{
		ya_printf(C_LOG_INFO,"\n\r%s create smartconfig thread failed", __FUNCTION__);
		return -1;
	}			
	return 0;
}

int stop_smnt(void)
{
	if(!run_smartconfig)
		return 0;
	
	run_smartconfig = 0;
	ya_wifi_stop_monitor();

	free_node_info();

	if (sconfig_info != NULL)
		ya_hal_os_memory_free(sconfig_info);

	if(ya_parse_to_ds != NULL)
		ya_hal_os_memory_free(ya_parse_to_ds);

	if(ya_parse_from_ds != NULL)
		ya_hal_os_memory_free(ya_parse_from_ds);

	if(ya_parse_mul != NULL)
		ya_hal_os_memory_free(ya_parse_mul);

	info_head = NULL;
	sconfig_info = NULL;
	smartconfig_gobal.get_result_callback=NULL;
	
	ya_hal_os_thread_delete(&smart_config_task);

	return 0;
}
