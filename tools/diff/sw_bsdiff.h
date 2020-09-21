#ifndef __SW_BSDIFF_H__
#define __SW_BSDIFF_H__

int SwBsdiff(const unsigned char *oldBuff, unsigned int oldLen, const unsigned char *newBuff, unsigned int newLen,
    unsigned char *outBuff, unsigned int *outLen);

int SwLzmaCompress(unsigned char *src, size_t srcLen, char *dst, size_t *dstLen);

unsigned char * SwGetLzmaProp();

#endif

