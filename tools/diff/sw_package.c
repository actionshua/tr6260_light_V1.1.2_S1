#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sw_package.h"
#include "sw_config.h"
#include "sw_sha256.h"
#include "sw_crc32.h"
#include "sw_bsdiff.h"

#define SW_PATCH_MAX_LEN (20 * 1024)
PkgHeader g_pkgHeader;
SectionOpt g_pkgOpt[OTA_DIFF_SECTION_NUM];
unsigned char g_pkgBuff[1*1024*1024];
unsigned int g_pkgTableLen;
unsigned int g_pkgPatchLen;

PkgHeader *SwPkgHeaderGet()
{
    return &g_pkgHeader;
}

SectionOpt *SwPkgSectionTableGet()
{
    return g_pkgOpt;
}

static int SwPkgSigntureAdd()
{
#if 0
    SectionTable sign;
    unsigned char hash[32];
    int i;

    memset(&sign, 0, sizeof(sign));
    memcpy(sign.sectionName, "signature", strlen("signature"));
    sign.patchOffset = g_pkgPatchLen;
    sign.patchSize = 32;
    SwSha256(g_pkgHeader, sizeof(g_pkgHeader), hash);
    memcpy(g_pkgBuff + g_pkgTableLen, &sign, sizeof(sign));
    g_pkgTableLen += sizeof(sign);
    memcpy(g_pkgBuff + g_pkgPatchLen, hash, sizeof(hash));
    g_pkgPatchLen += sizeof(hash);
#endif
    return 0;
}

int SwPkgFileCreate()
{
    int fd = -1;

    memcpy(g_pkgBuff, &g_pkgHeader, sizeof(g_pkgHeader));
    g_pkgHeader.crc = SwCrc32Calc(g_pkgBuff + 16, g_pkgPatchLen - 16);
    g_pkgHeader.pkgSize = g_pkgPatchLen;
    memcpy(g_pkgBuff, &g_pkgHeader, sizeof(g_pkgHeader));
    system("rm -rf pkgsrc");
    fd = open("pkgsrc", O_RDWR | O_CREAT | 0750);
    if (fd < 0) {
        printf("open pkgsrc failed.\n");
        return -1;
    }
    
    write(fd, g_pkgBuff, g_pkgPatchLen);
    close(fd);
    system("chmod 0777 pkgsrc");

    return 0;
}

static int SwPkgDiffTotalCreate(SectionOpt *sectionInfo)
{
    int fd;
    unsigned char *patchSlice = NULL;
    unsigned char *patch = NULL;
    size_t patchLen;
    unsigned int readLen;
    unsigned int tmpLen;

    patchSlice = malloc(sectionInfo->partSize + 1);
    if (patchSlice == NULL) {
        printf("%s[%d] No Mem Left\n", __FUNCTION__, __LINE__);
        return -1;
    }
    memset(patchSlice, 0, sectionInfo->partSize + 1);

    sectionInfo->sectionTable[sectionInfo->sectionNum - 1].method = 0;
    fd = open(sectionInfo->newFirmName, O_RDONLY);
    if (fd < 0) {
        printf("%s[%d] open %s failed\n", __FUNCTION__, __LINE__, sectionInfo->newFirmName);
        free(patchSlice);
        return -1;
    }

    lseek(fd, (sectionInfo->sectionNum - 1) * sectionInfo->partSize, SEEK_SET);
    readLen = read(fd, patchSlice, sectionInfo->partSize);
    if (readLen <= 0) {
        printf("%s[%d] read %s failed\n", __FUNCTION__, __LINE__, sectionInfo->newFirmName);
        free(patchSlice);
        close(fd);
        return -1;
    }

    patch = malloc(sectionInfo->partSize + 1);
    if (patch == NULL) {
        printf("%s[%d] No Mem Left\n", __FUNCTION__, __LINE__);
        free(patchSlice);
        close(fd);
        return -1;
    }
    memset(patch, 0, sectionInfo->partSize + 1);
    patchLen = sectionInfo->partSize;
    if (SwLzmaCompress(patchSlice, readLen, patch, &patchLen) != 0) {
        free(patch);
        free(patchSlice);
        close(fd);
        return -1;
    }

    sectionInfo->sectionTable[sectionInfo->sectionNum - 1].addrbLen = readLen;
    SwSha256(patch, readLen, sectionInfo->sectionTable[sectionInfo->sectionNum - 1].hashb);
    sectionInfo->sectionTable[sectionInfo->sectionNum - 1].patchSize = patchLen;
    if (patchLen > SW_PATCH_MAX_LEN) {
        printf("%s[%d] patch size(0x%lx) bigger than 0x%x, please adjust the step size\n", __FUNCTION__, __LINE__, patchLen, SW_PATCH_MAX_LEN);
        printf("%s[%d] WARNING !!!!\n", __FUNCTION__, __LINE__);
        printf("%s[%d] WARNING !!!!\n", __FUNCTION__, __LINE__);
        printf("%s[%d] WARNING !!!!\n", __FUNCTION__, __LINE__);
    }
    memcpy(sectionInfo->sectionTable[sectionInfo->sectionNum - 1].reserve, SwGetLzmaProp(), 5);

    memcpy(g_pkgBuff + g_pkgTableLen, &sectionInfo->sectionTable[sectionInfo->sectionNum - 1], sizeof(SectionTable));
    g_pkgTableLen += sizeof(SectionTable);
    tmpLen = (patchLen % 16) ? (patchLen + 16 - patchLen % 16) : patchLen;
    memcpy(g_pkgBuff + g_pkgPatchLen, patch, patchLen);
    g_pkgPatchLen += tmpLen;

    free(patch);
    free(patchSlice);
    close(fd);
    return 0;
}

static int SwPkgDiffPatchCreate(SectionOpt *sectionInfo)
{
    unsigned char *oldSlice = NULL;
    unsigned char *newSlice = NULL;
    unsigned char *patchSlice = NULL;
    int oldFd = -1;
    int newFd = -1;
    unsigned int oldLen;
    unsigned int newLen;
    size_t patchLen;
    unsigned int tmpLen;
    int i;
    int oldover = 0;

    oldSlice = malloc(sectionInfo->partSize + 1);
    if (oldSlice == NULL) {
        printf("%s[%d] No Mem Left\n", __FUNCTION__, __LINE__);
        return -1;
    }

    newSlice = malloc(sectionInfo->partSize + 1);
    if (newSlice == NULL) {
        printf("%s[%d] No Mem Left\n", __FUNCTION__, __LINE__);
        free(oldSlice);
        return -1;
    }
#if 0
    patchSlice = malloc(sectionInfo->partSize + 1);
    if (patchSlice == NULL) {
        printf("%s[%d] No Mem Left\n", __FUNCTION__, __LINE__);
        return -1;
    }
#endif
    // use for uncompress patch case
    patchSlice = malloc(2 * sectionInfo->partSize + 1);
    if (patchSlice == NULL) {
        printf("%s[%d] No Mem Left\n", __FUNCTION__, __LINE__);
        free(newSlice);
        free(oldSlice);
        return -1;
    }

    oldFd = open(sectionInfo->oldFirmName, O_RDONLY);
    if (oldFd < 0) {
        printf("%s[%d] open %s failed\n", __FUNCTION__, __LINE__, sectionInfo->oldFirmName);
        free(newSlice);
        free(oldSlice);
        free(patchSlice);
        return -1;
    }

    newFd = open(sectionInfo->newFirmName, O_RDONLY);
    if (newFd < 0) {
        printf("%s[%d] open %s failed\n", __FUNCTION__, __LINE__, sectionInfo->newFirmName);
        close(oldFd);
        free(newSlice);
        free(oldSlice);
        free(patchSlice);
        return -1;
    }

    for (i = 0; i < sectionInfo->sectionNum; i++) {
        memcpy(sectionInfo->sectionTable[i].sectionName, sectionInfo->sectionName, sizeof(sectionInfo->sectionTable[i].sectionName));
        sectionInfo->sectionTable[i].method = sectionInfo->method;
        sectionInfo->sectionTable[i].compress = sectionInfo->compress;
        sectionInfo->sectionTable[i].addrf = sectionInfo->addrf + sectionInfo->partSize * i;
        sectionInfo->sectionTable[i].addrb = sectionInfo->addrb + sectionInfo->partSize * i;
        sectionInfo->sectionTable[i].patchOffset = g_pkgPatchLen;

        memset(oldSlice, 0, sectionInfo->partSize + 1);
        memset(newSlice, 0, sectionInfo->partSize + 1);
        memset(patchSlice, 0, 2 * sectionInfo->partSize + 1);

        oldLen = read(oldFd, oldSlice, sectionInfo->partSize);
        if (oldLen <= 0) {
            oldover = 1;
        }

        newLen = read(newFd, newSlice, sectionInfo->partSize);
        if (newLen <= 0) {
            printf("%s[%d] read %s failed\n", __FUNCTION__, __LINE__, sectionInfo->newFirmName);
            close(newFd);
            close(oldFd);
            free(newSlice);
            free(oldSlice);
            free(patchSlice);
            return -1;
        }

        if (oldover != 1) {
            sectionInfo->sectionTable[i].addrfLen = oldLen;
            SwSha256(oldSlice, oldLen, sectionInfo->sectionTable[i].hashf);
        }

        sectionInfo->sectionTable[i].addrbLen = newLen;
        SwSha256(newSlice, newLen, sectionInfo->sectionTable[i].hashb);
        patchLen = sectionInfo->partSize;
        if (oldover == 1) {
            sectionInfo->sectionTable[i].method = 0;
            if (SwLzmaCompress(newSlice, newLen, patchSlice, &patchLen) != 0) {
                close(newFd);
                close(oldFd);
                free(newSlice);
                free(oldSlice);
                free(patchSlice);
                return -1;
            }
        } else {
            if (SwBsdiff(oldSlice, oldLen, newSlice, newLen, patchSlice, (unsigned int *)&patchLen) != 0) {
                close(newFd);
                close(oldFd);
                free(newSlice);
                free(oldSlice);
                free(patchSlice);
                return -1;
            }
        }
        memcpy(sectionInfo->sectionTable[i].reserve, SwGetLzmaProp(), 5);
        sectionInfo->sectionTable[i].patchSize = patchLen;

        memcpy(g_pkgBuff + g_pkgTableLen, &sectionInfo->sectionTable[i], sizeof(SectionTable));
        g_pkgTableLen += sizeof(SectionTable);
        tmpLen = (patchLen % 16) ? (patchLen + 16 - patchLen % 16) : patchLen;
        memcpy(g_pkgBuff + g_pkgPatchLen, patchSlice, patchLen);
        g_pkgPatchLen += tmpLen;
    }

    close(newFd);
    close(oldFd);

    free(newSlice);
    free(oldSlice);
    free(patchSlice);
    return 0;
}

static int SwPkgTotalPatchCreate(SectionOpt *sectionInfo)
{
    unsigned char *newSlice = NULL;
    unsigned char *patchSlice = NULL;
    int newFd = -1;
    unsigned int newLen;
    size_t patchLen;
    unsigned int tmpLen;
    struct stat fileStat;
    int i;

    memset(&fileStat, 0, sizeof(fileStat));
    stat(sectionInfo->newFirmName, &fileStat);

    patchSlice = malloc(fileStat.st_size + 1);
    if (patchSlice == NULL) {
        printf("%s[%d] No Mem Left\n", __FUNCTION__, __LINE__);
        return -1;
    }

    newSlice = malloc(fileStat.st_size + 1);
    if (newSlice == NULL) {
        printf("%s[%d] No Mem Left\n", __FUNCTION__, __LINE__);
        free(patchSlice);
        return -1;
    }
    
    newFd = open(sectionInfo->newFirmName, O_RDONLY);
    if (newFd < 0) {
        printf("%s[%d] open %s failed\n", __FUNCTION__, __LINE__, sectionInfo->newFirmName);
        free(newSlice);
        free(patchSlice);
        return -1;
    }

    for (i = 0; i < sectionInfo->sectionNum; i++) {
        memcpy(sectionInfo->sectionTable[i].sectionName, sectionInfo->sectionName, sizeof(sectionInfo->sectionTable[i].sectionName));
        sectionInfo->sectionTable[i].method = sectionInfo->method;
        sectionInfo->sectionTable[i].compress = sectionInfo->compress;
        sectionInfo->sectionTable[i].addrf = sectionInfo->addrf + sectionInfo->partSize * i;
        sectionInfo->sectionTable[i].addrb = sectionInfo->addrb + sectionInfo->partSize * i;
        sectionInfo->sectionTable[i].patchOffset = g_pkgPatchLen;

        memset(newSlice, 0, fileStat.st_size + 1);
        if (sectionInfo->partSize) {
            newLen = read(newFd, newSlice, sectionInfo->partSize);
        } else {
            newLen = read(newFd, newSlice, fileStat.st_size);
        }

        if (newLen < 0) {
            printf("%s[%d] read %s failed\n", __FUNCTION__, __LINE__, sectionInfo->newFirmName);
            close(newFd);
            free(newSlice);
            free(patchSlice);
            return -1;
        }

        SwSha256(newSlice, newLen, sectionInfo->sectionTable[i].hashb);
        sectionInfo->sectionTable[i].addrbLen = newLen;
        if (sectionInfo->compress) {
            patchLen = fileStat.st_size;
            if (SwLzmaCompress(newSlice, newLen, patchSlice, &patchLen) != 0) {
                close(newFd);
                free(newSlice);
                free(patchSlice);
                return -1;
            }
            memcpy(sectionInfo->sectionTable[i].reserve, SwGetLzmaProp(), 5);
            sectionInfo->sectionTable[i].patchSize = patchLen;
            tmpLen = (patchLen % 16) ? (patchLen + 16 - (patchLen % 16)) : patchLen;
            memcpy(g_pkgBuff + g_pkgPatchLen, patchSlice, patchLen);
            g_pkgPatchLen += tmpLen;

            if (patchLen > SW_PATCH_MAX_LEN) {
                printf("%s[%d] patch size(0x%lx) bigger than 0x%x, please adjust the step size\n", __FUNCTION__, __LINE__, patchLen, SW_PATCH_MAX_LEN);
                printf("%s[%d] WARNING !!!!\n", __FUNCTION__, __LINE__);
                printf("%s[%d] WARNING !!!!\n", __FUNCTION__, __LINE__);
                printf("%s[%d] WARNING !!!!\n", __FUNCTION__, __LINE__);
            }
        } else {
            sectionInfo->sectionTable[i].patchSize = newLen;
            tmpLen = (newLen % 16) ? (newLen + 16 - (newLen % 16)) : newLen;
            memcpy(g_pkgBuff + g_pkgPatchLen, newSlice, newLen);
            g_pkgPatchLen += tmpLen;

            if (newLen > SW_PATCH_MAX_LEN) {
                printf("%s[%d] patch size(0x%x) bigger than 0x%x, please adjust the step size\n", __FUNCTION__, __LINE__, newLen, SW_PATCH_MAX_LEN);
                printf("%s[%d] WARNING !!!!\n", __FUNCTION__, __LINE__);
                printf("%s[%d] WARNING !!!!\n", __FUNCTION__, __LINE__);
                printf("%s[%d] WARNING !!!!\n", __FUNCTION__, __LINE__);
            }
        }

        memcpy(g_pkgBuff + g_pkgTableLen, &sectionInfo->sectionTable[i], sizeof(SectionTable));
        g_pkgTableLen += sizeof(SectionTable);
    }
    close(newFd);
    free(newSlice);
    free(patchSlice);
    return 0;
}

int main(int argc, char *argv[])
{
    int index;

    memset(g_pkgBuff, 0xFF, sizeof(g_pkgBuff));
    if (SwCfgParseInit() != 0) {
        return -1;
    }
    g_pkgTableLen = sizeof(g_pkgHeader);
    g_pkgPatchLen = g_pkgTableLen + sizeof(SectionTable) * g_pkgHeader.sectionNum;

    for (index = 0; index < OTA_DIFF_SECTION_NUM; index++) {
        if (g_pkgOpt[index].used == 0) {
            continue;
        }

        if (g_pkgOpt[index].method == 1) {
            if (SwPkgDiffPatchCreate(&g_pkgOpt[index]) != 0) {
                return -1;
            }
        } else {
            if (SwPkgTotalPatchCreate(&g_pkgOpt[index]) != 0) {
                return -1;
            }
        }
    }

    if (SwPkgSigntureAdd() != 0) {
        return -1;
    }

    if (SwPkgFileCreate() != 0 ) {
        return -1;
    }

    SwCfgParseDumpHeader(&g_pkgHeader);
    for (index = 0; index < OTA_DIFF_SECTION_NUM; index++) {
        if (g_pkgOpt[index].used == 0) {
            continue;
        }
        SwCfgParseDumpPartition(&g_pkgOpt[index]);
        SwCfgParseDumpSlice(&g_pkgOpt[index]);
    }

    return 0;
}
