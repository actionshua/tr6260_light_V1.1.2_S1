#ifndef __SW_PACKAGE_H__
#define __SW_PACKAGE_H__

#define OTA_DIFF_SECTION_NUM 64

typedef struct {
    unsigned char magic[7];
    unsigned char ver;
    unsigned int pkgSize;
    unsigned int crc;
    unsigned int sectionNum;
    unsigned char reserve[12];
    unsigned char srcVersion[16];
    unsigned char dstVersion[16];
} PkgHeader;

typedef struct {
    unsigned char sectionName[16];
    unsigned int addrf;
    unsigned int addrfLen;
    unsigned int addrb;
    unsigned int addrbLen;
    unsigned int patchOffset;
    unsigned int patchSize;
    unsigned char method;
    unsigned char compress;
    unsigned char reserve[6];
    unsigned char hashf[32];
    unsigned char hashb[32];
} SectionTable;

typedef struct {
    int used;
    unsigned char sectionName[64];
    int partSize;
    int sectionNum;
    unsigned int addrf;
    unsigned int addrfLen;
    unsigned int addrb;
    unsigned int addrbLen;
    unsigned char method;
    unsigned char compress;
    unsigned char oldFirmName[64];
    unsigned char newFirmName[64];
    SectionTable sectionTable[OTA_DIFF_SECTION_NUM];
} SectionOpt;

PkgHeader *SwPkgHeaderGet();

SectionOpt *SwPkgSectionTableGet();

#endif
