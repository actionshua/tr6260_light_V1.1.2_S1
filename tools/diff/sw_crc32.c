#include <stdio.h>

#if 0
static unsigned long g_crcTable[256];

/*
 * reverse the binary bits, like 1000 0101b to 1010 0001b
 */
static unsigned long SwCrc32BitReverse(unsigned long data, int width)
{
    int i;
    unsigned long var = 0;

    for (i = 0; i < width; i++) {
        if (data & 0x01) {
            var |= 1 << (width - 1 - i);
        }
        data >>= 1;
    }

    return var;
}

static void SwCrc32InitTable()
{
    unsigned int gx = 0x04c11db7;
    unsigned int temp, crc;
    int i, j;

    for (i = 0; i <= 0xFF; i++) {
        temp = SwCrc32BitReverse(i, 8);
        g_crcTable[i] = temp << 24;
        for (j = 0; j < 8; j++) {
            unsigned long int t1, t2;
            unsigned long int flag = g_crcTable[i] & 0x80000000;
            t1 = (g_crcTable[i] << 1);
            if (flag == 0) {
                t2 = 0;
            } else {
                t2 = gx;
            }
            g_crcTable[i] = t1 ^ t2;
        }
        crc = g_crcTable[i];
        g_crcTable[i] = SwCrc32BitReverse(g_crcTable[i], 32);
    }
}

unsigned long SwCrc32Calc(unsigned char *data, int dataLen)
{
    static int tableInit = 0;
    unsigned int crc = 0xffffffff;
    unsigned char *p = data;
    int i;

    if (tableInit == 0) {
        SwCrc32InitTable();
        tableInit = 1;
    }

    for (i = 0; i < dataLen; i++) {
        crc = g_crcTable[(crc ^ (*(p + i))) & 0xff] ^ (crc >> 8);
    }

    return  ~crc;
}

#if 0
void main()
{
    unsigned long crc;
    SwCrc32InitTable();

    crc = SwCrc32Calc("1234567890", 10);

    printf("CRC32=%08X\n",crc);
}
#endif
#endif

#define POLY  0x04C11DB7

static unsigned int reflect(unsigned int ref, unsigned char ch)
{
    int i;
    unsigned int value = 0;
    for (i = 1; i < (ch + 1); i++) {
        if (ref & 1) {
            value |= 1 << (ch - i);
        }
        ref >>= 1;
    }
    return value;
}

unsigned int SwCrc32Calc(unsigned char *ptr, unsigned int len)
{
    unsigned char i;
    unsigned int crc = 0xffffffff;
    while (len--) {
        for (i = 1; i != 0; i <<= 1) {
            if ((crc & 0x80000000) != 0) {
                crc <<= 1;
                crc ^= POLY;
            } else {
                crc <<= 1;
            }
            if ((*ptr & i) != 0) {
                crc ^= POLY;
            }
        }
        ptr++;
    }
    return (reflect(crc, 32) ^ 0xffffffff);
}

