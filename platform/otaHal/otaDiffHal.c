#include "system_common.h"
#include "drv_spiflash.h"
#include "wdt.h"

typedef struct {
    unsigned char header[3];
    unsigned char len;
    unsigned int  crc;
    unsigned int  patch_addr;
    unsigned int  patch_size;
    unsigned char ota_diff_update_flag;
    unsigned char ota_diff_update_location;
    unsigned char ota_diff_update_state;
    unsigned char reserved[5];  
} OtaState;

typedef struct {
    unsigned char magic[7];
    unsigned char patch_version;
    unsigned int  size;
    unsigned int  crc;
    unsigned int  sec_num;
    unsigned char reserved_1[12];
    unsigned char source_version[16];
    unsigned char target_version[16];
} OtaPkgHeader;

typedef struct {
    unsigned char init;     // if initialized
    unsigned char get_first_frame;
    unsigned int  s_addr;   // start addr
    unsigned int  e_addr;   // end addr
    unsigned int  offset;   // write offset.
    unsigned int  max_len;  // max size
    unsigned int  data_len; // firamware size
} OtaOperateInfo;
static OtaOperateInfo g_otaInfo;

#define OTA_STAT_PART_SIZE (4 * 1024)

unsigned int get_OTA_write_data_addr(void)
{
	return g_otaInfo.s_addr;
}

char otaHal_init(void)
{
    int ret;

    memset(&g_otaInfo, 0, sizeof(g_otaInfo));
    if (partion_info_get(PARTION_NAME_DATA_OTA, &g_otaInfo.s_addr, &g_otaInfo.max_len) != 0) {
        system_printf("can not get %s info\n", PARTION_NAME_DATA_OTA);
        return -1;
    }
    system_printf("ota diff part %x size %x\n", g_otaInfo.s_addr, g_otaInfo.max_len);

    // erase flash.
    if ((ret = hal_spiflash_erase(g_otaInfo.s_addr, g_otaInfo.max_len)) != 0) {
        system_printf("erase flash failed(%d), when ota download request.....", ret);
        return -1;
    }
    g_otaInfo.e_addr = g_otaInfo.s_addr + g_otaInfo.max_len;
    g_otaInfo.init = 1;

    return 0;
}

char otaHal_write(const unsigned char * data, unsigned short len)
{
    OtaPkgHeader *header = NULL;
    unsigned int tmpFileLen = 0;

    //first frame handle
    if (g_otaInfo.get_first_frame == 0) {
        if (len < sizeof(OtaPkgHeader)) {
            system_printf("len=%d headersize=%d\n", len, sizeof(OtaPkgHeader));
            return -1;
        }

        header = (OtaPkgHeader *)data;
        g_otaInfo.data_len = header->size;
        system_printf("pkgSize=%d\n", header->size);
        tmpFileLen = (header->size % OTA_STAT_PART_SIZE) ? (header->size + OTA_STAT_PART_SIZE - header->size % OTA_STAT_PART_SIZE) : header->size;
        if ((tmpFileLen + 2 * OTA_STAT_PART_SIZE) > g_otaInfo.max_len) {
            system_printf("ota size(%d) not enough need %d\n", g_otaInfo.max_len, tmpFileLen + 2 * OTA_STAT_PART_SIZE);
            return -1;
        }
        g_otaInfo.s_addr = g_otaInfo.s_addr + g_otaInfo.max_len - tmpFileLen - 2 * OTA_STAT_PART_SIZE;
        g_otaInfo.get_first_frame = 1;
        system_printf("saddr=0x%x datalen=0x%x alignFileLen=0x%x\n", g_otaInfo.s_addr, g_otaInfo.data_len, tmpFileLen);
    }

    if (g_otaInfo.offset + len > g_otaInfo.data_len) {
        system_printf("offset=%d len=%d datalen=%d\n", g_otaInfo.offset, len, g_otaInfo.data_len);
        return -1;
    }
    hal_spifiash_write(g_otaInfo.s_addr + g_otaInfo.offset, (unsigned char *)data, len);
    g_otaInfo.offset += len;

    return 0;
}

extern uint32_t crc32(uint32_t crc, const void *buf, size_t size);
char otaHal_done(void)
{
    OtaState state;
    int ret;
    char *pState = (char *)&state;

    memset(&state, 0xFF, sizeof(state));
    state.len = 16;
    memcpy(state.header, "OTA", 3);
    state.ota_diff_update_flag = 1;
    state.patch_addr = g_otaInfo.s_addr;
    state.patch_size = g_otaInfo.data_len;

    if ((g_otaInfo.data_len != 0) && (g_otaInfo.offset == g_otaInfo.data_len)) {
        state.crc = crc32(0, pState + 8, sizeof(state) - 8);
        system_printf("offset=%x datalen=%x\n", g_otaInfo.offset, g_otaInfo.data_len);
        system_printf("crc=%x\n", state.crc);

        hal_spifiash_write(g_otaInfo.e_addr - 2 * OTA_STAT_PART_SIZE, (unsigned char *)&state, sizeof(state));
        hal_spifiash_write(g_otaInfo.e_addr - OTA_STAT_PART_SIZE, (unsigned char *)&state, sizeof(state));
        system_printf("ota begin to reboot system!!!!");
    } else {
        system_printf("ota download not completely. fileLen=0x%x downloadLen=0x%x\n", g_otaInfo.data_len,  g_otaInfo.offset);
    }

    system_reset(STARTUP_TYPE_OTA);
    return 0;
}

