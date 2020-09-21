#include "ya_common.h"
#include <string.h>
#include "wifi_80211.h"


#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
extern "C"
{
#endif

#ifndef offsetof
/** offset address of the struct member */
#define offsetof(type, member) ((long) &((type *) 0)->member)
#endif

int os_is_big_endian(void)
{
    uint32_t data = 0xFF000000;

    if (0xFF == *(uint8_t *) & data) {
        return 1;                                    /* big endian */
    }

    return 0;                                         /* little endian */
}

/* reverse byte order */
static uint16_t reverse_16bit(uint16_t data)
{
    return (data >> 8) | (data << 8);
}


/* host byte order to little endian */
static uint16_t os_htole16(uint16_t data)
{
    if (os_is_big_endian()) {
        return reverse_16bit(data);
    }

    return data;
}

/* little endian to host byte order */
static uint16_t os_le16toh(uint16_t data)
{
    return os_htole16(data);
}

/* get unaligned data in little endian. */
static uint16_t os_get_unaligned_le16(uint8_t * ptr)
{
    uint16_t res;

    memcpy(&res, ptr, sizeof(uint16_t));

    return os_le16toh(res);

}


/**
 * wifi_80211_is_mgmt - check if type is IEEE80211_FTYPE_MGMT
 * @fc: frame control bytes in little-endian byteorder
 */
#if 0
int wifi_80211_is_mgmt(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_FTYPE)) ==
           os_htole16(IEEE80211_FTYPE_MGMT);
}
#endif

/**
 * wifi_80211_is_ctl - check if type is IEEE80211_FTYPE_CTL
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_is_ctl(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_FTYPE)) ==
           os_htole16(IEEE80211_FTYPE_CTL);
}

/**
 * wifi_80211_is_data - check if type is IEEE80211_FTYPE_DATA
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_is_data(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_FTYPE)) ==
           os_htole16(IEEE80211_FTYPE_DATA);
}


/**
 * wifi_80211_has_tods - check if IEEE80211_FCTL_TODS is set
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_has_tods(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_TODS)) != 0;
}

int wifi_80211_is_retry(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_RETRY)) != 0;
}


/**
 * wifi_80211_has_fromds - check if IEEE80211_FCTL_FROMDS is set
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_has_fromds(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_FROMDS)) != 0;
}

/**
 * wifi_80211_has_a4 - check if IEEE80211_FCTL_TODS and IEEE80211_FCTL_FROMDS are set
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_has_a4(uint16_t fc)
{
    uint16_t tmp = os_htole16(IEEE80211_FCTL_TODS | IEEE80211_FCTL_FROMDS);
    return (fc & tmp) == tmp;
}

/**
 * wifi_80211_has_order - check if IEEE80211_FCTL_ORDER is set
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_has_order(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_ORDER)) != 0;
}

/**
 * wifi_80211_has_protected - check if IEEE80211_FCTL_PROTECTED is set
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_has_protected(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_PROTECTED)) != 0;
}

/**
 * wifi_80211_is_data_qos - check if type is IEEE80211_FTYPE_DATA and IEEE80211_STYPE_QOS_DATA is set
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_is_data_qos(uint16_t fc)
{
    /*
     * mask with QOS_DATA rather than IEEE80211_FCTL_STYPE as we just need
     * to check the one bit
     */
    return (fc & os_htole16(IEEE80211_FCTL_FTYPE | IEEE80211_STYPE_QOS_DATA)) ==
           os_htole16(IEEE80211_FTYPE_DATA | IEEE80211_STYPE_QOS_DATA);
}

/**
 * wifi_80211_is_data_present - check if type is IEEE80211_FTYPE_DATA and has data
 * @fc: frame control bytes in little-endian byteorder
 */
#if 0
int wifi_80211_is_data_present(uint16_t fc)
{
    /*
     * mask with 0x40 and test that that bit is clear to only return true
     * for the data-containing substypes.
     */
    return (fc & os_htole16(IEEE80211_FCTL_FTYPE | 0x40)) ==
           os_htole16(IEEE80211_FTYPE_DATA);
}
#endif

/**
 * wifi_80211_is_data_present - check if type is IEEE80211_FTYPE_DATA and only data
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_is_data_exact(uint16_t fc)
{
    uint16_t tmp = fc & os_htole16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE);

    return (tmp == os_htole16(IEEE80211_FTYPE_DATA)) ||
           (tmp == os_htole16(IEEE80211_FTYPE_DATA | IEEE80211_STYPE_QOS_DATA));
}

/**
 * wifi_80211_is_beacon - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_BEACON
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_is_beacon(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
           os_htole16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_BEACON);
}

/**
 * wifi_80211_is_action - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_ACTION
 * @fc: frame control bytes in little-endian byteorder
 */
#if 0
int wifi_80211_is_action(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
           os_htole16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ACTION);
}
#endif

/**
 * wifi_80211_is_probe_req - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_PROBE_REQ
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_is_probe_req(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
           os_htole16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_PROBE_REQ);
}

/**
 * wifi_80211_is_probe_resp - check if IEEE80211_FTYPE_MGMT && IEEE80211_STYPE_PROBE_RESP
 * @fc: frame control bytes in little-endian byteorder
 */
int wifi_80211_is_probe_resp(uint16_t fc)
{
    return (fc & os_htole16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
           os_htole16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_PROBE_RESP);
}


/**
 * wifi_80211_get_SA - get pointer to SA
 * @hdr: the frame
 *
 * Given an 802.11 frame, this function returns the offset
 * to the source address (SA). It does not verify that the
 * header is long enough to contain the address, and the
 * header must be long enough to contain the frame control
 * field.
 */
uint8_t *wifi_80211_get_SA(struct wifi_80211_hdr *hdr)
{
    if (wifi_80211_has_a4(hdr->frame_control))
        return hdr->addr4;
    if (wifi_80211_has_fromds(hdr->frame_control))
        return hdr->addr3;
    return hdr->addr2;
}

/**
 * wifi_80211_get_DA - get pointer to DA
 * @hdr: the frame
 *
 * Given an 802.11 frame, this function returns the offset
 * to the destination address (DA). It does not verify that
 * the header is long enough to contain the address, and the
 * header must be long enough to contain the frame control
 * field.
 */
uint8_t *wifi_80211_get_DA(struct wifi_80211_hdr *hdr)
{
    if (wifi_80211_has_tods(hdr->frame_control))
        return hdr->addr3;
    else
        return hdr->addr1;
}

uint8_t *wifi_80211_get_BSSID(struct wifi_80211_hdr *hdr)
{
    if (wifi_80211_has_tods(hdr->frame_control)) {
        if (!wifi_80211_has_fromds(hdr->frame_control))
            return hdr->addr1;
        else
            return NULL;
    } else {
        if (wifi_80211_has_fromds(hdr->frame_control))
            return hdr->addr2;
        else
            return hdr->addr3;
    }
}

int wifi_80211_get_bssid(uint8_t *in, uint8_t *mac)
{
    uint8_t *bssid = wifi_80211_get_BSSID((struct wifi_80211_hdr *)in);

    if (bssid)
        memcpy(mac, bssid, ETH_ALEN);
    else
        return -1;

    return 0;
}

int wifi_80211_has_frags(uint16_t fc)
{
    uint16_t tmp = fc & os_htole16(IEEE80211_FCTL_MOREFRAGS | IEEE80211_FCTL_ORDER);

    return !!tmp;
}

/* DATA:        24B */
/* QOS-DATA:    26B */
int wifi_80211_hdrlen(uint16_t fc)
{
    uint32_t hdrlen = 24;

    if (wifi_80211_is_data(fc)) {
        if (wifi_80211_has_a4(fc))
            hdrlen = 30;
        if (wifi_80211_is_data_qos(fc)) {
            hdrlen += IEEE80211_QOS_CTL_LEN;
            if (wifi_80211_has_order(fc))
                hdrlen += IEEE80211_HT_CTL_LEN;
        }
        goto out;
    }

    if (wifi_80211_is_ctl(fc)) {
        /*
         * ACK and CTS are 10 bytes, all others 16. To see how
         * to get this condition consider
         *   subtype mask:   0b0000000011110000 (0x00F0)
         *   ACK subtype:    0b0000000011010000 (0x00D0)
         *   CTS subtype:    0b0000000011000000 (0x00C0)
         *   bits that matter:         ^^^      (0x00E0)
         *   value of those: 0b0000000011000000 (0x00C0)
         */
        if ((fc & os_htole16(0x00E0)) == os_htole16(0x00C0))
            hdrlen = 10;
        else
            hdrlen = 16;
    }

out:
    return hdrlen;
}

/* helpers */
int wifi_80211_get_radiotap_len(uint8_t *data)
{
    struct wifi_80211_radiotap_header *hdr =
            (struct wifi_80211_radiotap_header *)data;

    return os_get_unaligned_le16((uint8_t *)&hdr->it_len);
}

const uint8_t *wifi_80211_find_ie(uint8_t eid, const uint8_t *ies, int len)
{
    while (len > 2 && ies[0] != eid) {
        len -= ies[1] + 2;
            ies += ies[1] + 2;
    }
    if (len < 2)
        return NULL;
    if (len < 2 + ies[1])
        return NULL;
    return ies;
}

/**
 * wifi_80211_find_vendor_ie - find vendor specific information element in data
 *
 * @oui: vendor OUI
 * @oui_type: vendor-specific OUI type
 * @ies: data consisting of IEs
 * @len: length of data
 *
 * Return: %NULL if the vendor specific element ID could not be found or if the
 * element is invalid (claims to be longer than the given data), or a pointer to
 * the first byte of the requested element, that is the byte containing the
 * element ID.
 *
 * Note: There are no checks on the element length other than having to fit into
 * the given data.
 */
const uint8_t *wifi_80211_find_vendor_ie(uint32_t oui, uint8_t oui_type, const uint8_t *ies, int len)
{
    struct wifi_80211_vendor_ie *ie;
    const uint8_t *pos = ies, *end = ies + len;
    int ie_oui;

    while (pos < end) {
        pos = wifi_80211_find_ie(WLAN_EID_VENDOR_SPECIFIC, pos,
                               end - pos);
        if (!pos)
            return NULL;

        ie = (struct wifi_80211_vendor_ie *)pos;

        /* make sure we can access ie->len */
        /* BUILD_BUG_ON(offsetof(struct wifi_80211_vendor_ie, len) != 1); */

        if (ie->len < sizeof(*ie))
            goto cont;

        ie_oui = ie->oui[0] << 16 | ie->oui[1] << 8 | ie->oui[2];
        /* awss_trace("oui=%x, type=%x, len=%d\r\n", ie_oui, oui_type, ie->len); */
        if (ie_oui == oui && ie->oui_type == oui_type)
            return pos;
cont:
        pos += 2 + ie->len;
    }
    return NULL;
}

/**
 * extract ssid from beacon frame or probe resp frame
 *
 * @beacon_frame: [IN] original 80211 beacon frame
 * @frame_len: [IN] len of beacon frame
 * @ssid: [OUT] null-terminated string, max len 32 bytes
 *
 * Return:
 *     0/success, -1/failed
 */
int wifi_80211_get_ssid(uint8_t *beacon_frame, uint16_t frame_len, uint8_t *ssid)
{
    uint16_t ieoffset = offsetof(struct wifi_80211_mgmt, u.beacon.variable);/* same as u.probe_resp.variable */
    const uint8_t *ptr = wifi_80211_find_ie(WLAN_EID_SSID,
                                          beacon_frame + ieoffset, frame_len - ieoffset);
    if (ptr) {
        uint8_t ssid_len = ptr[1];
        if (ssid_len <= 32) {    /* ssid 32 octets at most */
            memcpy(ssid, ptr + 2, ssid_len);/* eating EID & len */
            ssid[ssid_len] = '\0';
            return 0;
        }
    }

    return -1;
}

/**
 * extract channel from beacon frame or probe resp frame
 *
 * @beacon_frame: [IN] original 80211 beacon frame
 * @frame_len: [IN] len of beacon frame
 *
 * Return:
 *     bss channel 1-13, 0--means invalid channel
 */
int wifi_80211_get_bss_channel(uint8_t *beacon_frame, uint16_t frame_len)
{
    uint16_t ieoffset = offsetof(struct wifi_80211_mgmt, u.beacon.variable);/* same as u.probe_resp.variable */
    const uint8_t *ie = beacon_frame + ieoffset;
    uint16_t ielen = frame_len - ieoffset;

    const uint8_t *tmp;
    int channel_number = 0;

    tmp = wifi_80211_find_ie(WLAN_EID_DS_PARAMS, ie, ielen);
    if (tmp && tmp[1] == 1) {
        channel_number = tmp[2];
    } else {
        tmp = wifi_80211_find_ie(WLAN_EID_HT_OPERATION, ie, ielen);
        if (tmp && tmp[1] >= sizeof(struct wifi_80211_ht_operation)) {
            struct wifi_80211_ht_operation *htop = (void *)(tmp + 2);

            channel_number = htop->primary_chan;
        }
    }

    return channel_number;
}

static const uint8_t WPA_OUI23A_TYPE[] =            {0x00, 0x50, 0xf2, 0x01};
static const uint8_t RSN_SUITE_1X[] =               {0x00, 0x0f, 0xac, 0x01};

static const uint8_t WPA_CIPHER_SUITE_NONE23A[] =   {0x00, 0x50, 0xf2, 0x00};
static const uint8_t WPA_CIPHER_SUITE_WEP4023A[] =  {0x00, 0x50, 0xf2, 0x01};
static const uint8_t WPA_CIPHER_SUITE_TKIP23A[] =   {0x00, 0x50, 0xf2, 0x02};
/* static const uint8_t WPA_CIPHER_SUITE_WRAP23A[] = {0x00, 0x50, 0xf2, 0x03}; */
static const uint8_t WPA_CIPHER_SUITE_CCMP23A[] =   {0x00, 0x50, 0xf2, 0x04};
static const uint8_t WPA_CIPHER_SUITE_WEP10423A[] = {0x00, 0x50, 0xf2, 0x05};

static const uint8_t RSN_CIPHER_SUITE_NONE23A[] =   {0x00, 0x0f, 0xac, 0x00};
static const uint8_t RSN_CIPHER_SUITE_WEP4023A[] =  {0x00, 0x0f, 0xac, 0x01};
static const uint8_t RSN_CIPHER_SUITE_TKIP23A[] =   {0x00, 0x0f, 0xac, 0x02};
/* static const uint8_t RSN_CIPHER_SUITE_WRAP23A[] = {0x00, 0x0f, 0xac, 0x03}; */
static const uint8_t RSN_CIPHER_SUITE_CCMP23A[] =   {0x00, 0x0f, 0xac, 0x04};
static const uint8_t RSN_CIPHER_SUITE_WEP10423A[] = {0x00, 0x0f, 0xac, 0x05};

#define WPA_SELECTOR_LEN        (4)
#define RSN_SELECTOR_LEN        (4)

#define BIT(x)                  (1 << (x))
#define WPA_CIPHER_NONE         BIT(0)
#define WPA_CIPHER_WEP40        BIT(1)
#define WPA_CIPHER_WEP104       BIT(2)
#define WPA_CIPHER_TKIP         BIT(3)
#define WPA_CIPHER_CCMP         BIT(4)

static uint8_t map_cipher_to_encry(uint8_t cipher)
{
    switch (cipher) {
    case WPA_CIPHER_CCMP:
        return ENC_TYPE_AES;
    case WPA_CIPHER_TKIP:
        return ENC_TYPE_TKIP;
    case WPA_CIPHER_WEP40:
    case WPA_CIPHER_WEP104:
        return ENC_TYPE_WEP;
    case WPA_CIPHER_NONE:
        return ENC_TYPE_NONE;
    case (WPA_CIPHER_TKIP | WPA_CIPHER_CCMP):
        return ENC_TYPE_TKIPAES;
    default:
        printf("unknow cipher type: %x\r\n", cipher);
        return ENC_TYPE_INVALID;
    }
}

static int get_wpa_cipher_suite(const uint8_t *s)
{
    if (!memcmp(s, WPA_CIPHER_SUITE_NONE23A, WPA_SELECTOR_LEN))
        return WPA_CIPHER_NONE;
    if (!memcmp(s, WPA_CIPHER_SUITE_WEP4023A, WPA_SELECTOR_LEN))
        return WPA_CIPHER_WEP40;
    if (!memcmp(s, WPA_CIPHER_SUITE_TKIP23A, WPA_SELECTOR_LEN))
        return WPA_CIPHER_TKIP;
    if (!memcmp(s, WPA_CIPHER_SUITE_CCMP23A, WPA_SELECTOR_LEN))
        return WPA_CIPHER_CCMP;
    if (!memcmp(s, WPA_CIPHER_SUITE_WEP10423A, WPA_SELECTOR_LEN))
        return WPA_CIPHER_WEP104;

    return 0;
}

static int get_wpa2_cipher_suite(const uint8_t *s)
{
    if (!memcmp(s, RSN_CIPHER_SUITE_NONE23A, RSN_SELECTOR_LEN))
        return WPA_CIPHER_NONE;
    if (!memcmp(s, RSN_CIPHER_SUITE_WEP4023A, RSN_SELECTOR_LEN))
        return WPA_CIPHER_WEP40;
    if (!memcmp(s, RSN_CIPHER_SUITE_TKIP23A, RSN_SELECTOR_LEN))
        return WPA_CIPHER_TKIP;
    if (!memcmp(s, RSN_CIPHER_SUITE_CCMP23A, RSN_SELECTOR_LEN))
        return WPA_CIPHER_CCMP;
    if (!memcmp(s, RSN_CIPHER_SUITE_WEP10423A, RSN_SELECTOR_LEN))
        return WPA_CIPHER_WEP104;

    return 0;
}

static int cfg80211_parse_wpa_info(const uint8_t *wpa_ie, int wpa_ie_len,
                            uint8_t *group_cipher, uint8_t *pairwise_cipher,
                            uint8_t *is_8021x)
{
    int i, ret = 0;
    int left, count;
    const uint8_t *pos;

    if (wpa_ie_len <= 0) {
        /* No WPA IE - fail silently */
        return -1;
    }

    if (wpa_ie[1] != (uint8_t)(wpa_ie_len - 2))
        return -1;

    pos = wpa_ie;

    pos += 8;
    left = wpa_ie_len - 8;

    /* group_cipher */
    if (left >= WPA_SELECTOR_LEN) {

        *group_cipher = get_wpa_cipher_suite(pos);

        pos += WPA_SELECTOR_LEN;
        left -= WPA_SELECTOR_LEN;
    } else if (left > 0) {
        return -1;
    }

    /* pairwise_cipher */
    if (left >= 2) {
        /* count = le16_to_cpu(*(uint16_t*)pos); */
        count = os_get_unaligned_le16((uint8_t *)pos);
        pos += 2;
        left -= 2;

        if (count == 0 || left < count * WPA_SELECTOR_LEN) {
            return -1;
        }

        for (i = 0; i < count; i++) {
            *pairwise_cipher |= get_wpa_cipher_suite(pos);

            pos += WPA_SELECTOR_LEN;
            left -= WPA_SELECTOR_LEN;
        }
    } else if (left == 1) {
        return -1;
    }

    if (is_8021x) {
        if (left >= 6) {
            pos += 2;
            if (!memcmp(pos, WPA_OUI23A_TYPE, 4)) {
                *is_8021x = 1;
            }
        }
    }

    return ret;
}

static int cfg80211_parse_wpa2_info(const uint8_t* rsn_ie, int rsn_ie_len, uint8_t *group_cipher,
                             uint8_t *pairwise_cipher, uint8_t *is_8021x)
{
    int i, ret = 0;
    int left, count;
    const uint8_t *pos;

    if (rsn_ie_len <= 0) {
        /* No RSN IE - fail silently */
        return -1;
    }

    if (*rsn_ie != WLAN_EID_RSN || *(rsn_ie+1) != (uint8_t)(rsn_ie_len - 2)) {
        return -1;
    }

    pos = rsn_ie;
    pos += 4;
    left = rsn_ie_len - 4;

    /* group_cipher */
    if (left >= RSN_SELECTOR_LEN) {
        *group_cipher = get_wpa2_cipher_suite(pos);

        pos += RSN_SELECTOR_LEN;
        left -= RSN_SELECTOR_LEN;
    } else if (left > 0) {
        return -1;
    }

    /* pairwise_cipher */
    if (left >= 2) {
        /* count = le16_to_cpu(*(uint16_t*)pos); */
        count = os_get_unaligned_le16((uint8_t *)pos);
        pos += 2;
        left -= 2;

        if (count == 0 || left < count * RSN_SELECTOR_LEN) {
            return -1;
        }

        for (i = 0; i < count; i++) {
            *pairwise_cipher |= get_wpa2_cipher_suite(pos);

            pos += RSN_SELECTOR_LEN;
            left -= RSN_SELECTOR_LEN;
        }
    } else if (left == 1) {
        return -1;
    }

    if (is_8021x) {
        if (left >= 6) {
            pos += 2;
            if (!memcmp(pos, RSN_SUITE_1X, 4)) {
                *is_8021x = 1;
            }
        }
    }

    return ret;
}

/**
 * extract auth/encry type from beacon frame or probe resp frame
 *
 * @beacon_frame: [IN] original 80211 beacon frame
 * @frame_len: [IN] len of beacon frame
 *
 * Return:
 *     bss channel 1-13, 0--means invalid channel
 */
int wifi_80211_get_cipher_info(uint8_t *beacon_frame, uint16_t frame_len,
                             uint8_t *auth_type, uint8_t *pairwise_cipher_type,
                             uint8_t *group_cipher_type)
{
    struct wifi_80211_mgmt *mgmt = (struct wifi_80211_mgmt *)beacon_frame;
    uint8_t is_privacy = !!(mgmt->u.beacon.capab_info & WLAN_CAPABILITY_PRIVACY);

    uint16_t ieoffset = offsetof(struct wifi_80211_mgmt, u.beacon.variable);/* same as u.probe_resp.variable */
    const uint8_t *ie = beacon_frame + ieoffset;
    uint16_t ielen = frame_len - ieoffset;

    uint8_t auth = 0, group_cipher = 0, pairwise_cipher = 0, is80211X = 0;
    const uint8_t *tmp;
    int ret = 0;

    tmp = wifi_80211_find_ie(WLAN_EID_RSN, ie, ielen);
    if (tmp && tmp[1]) {
        ret = cfg80211_parse_wpa2_info(tmp, tmp[1] + 2, &group_cipher, &pairwise_cipher, &is80211X);
        if (is80211X)
            auth = AUTH_TYPE_WPA28021X;
        else
            auth = AUTH_TYPE_WPA2PSK;
        group_cipher = map_cipher_to_encry(group_cipher);
        pairwise_cipher = map_cipher_to_encry(pairwise_cipher);
    } else {
#ifdef SUPPORT_SMARTCONFIG_WPS
        tmp = wifi_80211_find_vendor_ie(WLAN_OUI_MICROSOFT, WLAN_OUI_TYPE_MICROSOFT_WPA, ie, ielen);
        if (tmp) {
            ret = cfg80211_parse_wpa_info(tmp, tmp[1] + 2, &group_cipher, &pairwise_cipher, &is80211X);
            if (is80211X)
                auth = AUTH_TYPE_WPA8021X;
            else
                auth = AUTH_TYPE_WPAPSK;
            group_cipher = map_cipher_to_encry(group_cipher);
            pairwise_cipher = map_cipher_to_encry(pairwise_cipher);
        } else
#endif
        {
            if (is_privacy) {
                auth = AUTH_TYPE_SHARED;  /* TODO: WEP */
                pairwise_cipher = ENC_TYPE_WEP;
                group_cipher = ENC_TYPE_WEP;
            } else {
                auth = AUTH_TYPE_OPEN;
                pairwise_cipher = ENC_TYPE_NONE;
                group_cipher = ENC_TYPE_NONE;
            }
        }
    }

    if (auth_type)
        *auth_type = auth;
    if (pairwise_cipher_type)
        *pairwise_cipher_type = pairwise_cipher;
    if (group_cipher_type)
        *group_cipher_type = group_cipher;

    return ret;
}

#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
}
#endif
