#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "sw_package.h"
#include "sw_config.h"

#define PARSE_ERR printf
PkgHeader  *g_pkgCfgHeader;
SectionOpt *g_pkgCfgOpt;

#define SW_PKG_CFG_FILENAME "swconfig.xml"

static int SwCfgParseGlobal(xmlDocPtr doc, xmlNodePtr curNode)
{
    xmlChar *key;
    int len;
    int value;

    if (doc == NULL || curNode == NULL) {
        PARSE_ERR("%s[%d] Invalid input paramters\n", __FUNCTION__, __LINE__);
        return -1;
    }

    curNode = curNode->xmlChildrenNode;
    while (curNode != NULL) {
        if (xmlStrcmp(curNode->name, BAD_CAST"magic") == 0) {
            key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
            //key = xmlNodeGetContent(curNode);
            len = sizeof(g_pkgCfgHeader->magic) > strlen(key) ? strlen(key) : sizeof(g_pkgCfgHeader->magic);
            memcpy(g_pkgCfgHeader->magic, key, len);
            xmlFree(key);
        }

        if (xmlStrcmp( curNode->name, BAD_CAST"patchver") == 0) {
            key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
            value = atoi(key);
            if (value < 0 || value > 0xFF) {
                PARSE_ERR("%s[%d] Invalid patch version %d\n", __FUNCTION__, __LINE__, value);
                xmlFree(key);
                return -1;
            }
            g_pkgCfgHeader->ver = value;
            xmlFree(key);
        }

        if (xmlStrcmp( curNode->name, BAD_CAST"softverf") == 0) {
            key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
            len = sizeof(g_pkgCfgHeader->srcVersion) > strlen(key) ? strlen(key) : sizeof(g_pkgCfgHeader->srcVersion);
            memcpy(g_pkgCfgHeader->srcVersion, key, len);
            xmlFree(key);
        }

        if (xmlStrcmp( curNode->name, BAD_CAST"softverb") == 0) {
            key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
            len = sizeof(g_pkgCfgHeader->dstVersion) > strlen(key) ? strlen(key) : sizeof(g_pkgCfgHeader->dstVersion);
            memcpy(g_pkgCfgHeader->dstVersion, key, len);
            xmlFree(key);
        }

        curNode = curNode->next;
    }

    return 0;
}

static int SwCfgUpdateElement(SectionOpt *pkgCfgOpt)
{
    struct stat fileStat;
    size_t fileLen;
    int slice;

    //g_pkgCfgHeader->sectionNum = 1; // signture
    if (pkgCfgOpt->partSize == 0) {
        pkgCfgOpt->sectionNum++;
        g_pkgCfgHeader->sectionNum++;
        return 0;
    }

    if (strlen(pkgCfgOpt->newFirmName) == 0) {
        PARSE_ERR("%s[%d] Invalid filename\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (stat(pkgCfgOpt->newFirmName, &fileStat) != 0) {
        PARSE_ERR("%s[%d] Invalid filename\n", __FUNCTION__, __LINE__);
        return -1;
    }
    fileLen = fileStat.st_size;
    slice = fileLen / pkgCfgOpt->partSize;

    if (fileLen % pkgCfgOpt->partSize != 0) {
        slice += 1;
    }

    pkgCfgOpt->sectionNum = slice;
    g_pkgCfgHeader->sectionNum += pkgCfgOpt->sectionNum;

    return 0;
}

static int SwCfgParseElement(xmlDocPtr doc, xmlNodePtr curNode)
{
    xmlChar *key;
    xmlChar *order;
    int value;
    int len;
    int orderNum;

    if (doc == NULL || curNode == NULL) {
        PARSE_ERR("%s[%d] Invalid input paramters\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (xmlStrcmp(curNode->name, BAD_CAST"global") == 0) {
        return SwCfgParseGlobal(doc, curNode);
    }

    order = xmlGetProp(curNode, "order");
    if (order == NULL) {
        PARSE_ERR("%s[%d] Node error need order attr\n", __FUNCTION__, __LINE__);
        return -1;
    }

    orderNum = atoi(order);
    if (orderNum < 0 || orderNum > 32) {
        PARSE_ERR("%s[%d] Invalid section %d\n", __FUNCTION__, __LINE__, orderNum);
        return -1;
    }

    if (g_pkgCfgOpt[orderNum].used == 1) {
        PARSE_ERR("%s[%d] duplicate section %d\n", __FUNCTION__, __LINE__, orderNum);
        return -1;
    }

    g_pkgCfgOpt[orderNum].used = 1;
    //g_pkgCfgHeader->sectionNum++;
    len = sizeof(g_pkgCfgOpt[orderNum].sectionName) > strlen(curNode->name) ? strlen(curNode->name) : sizeof(g_pkgCfgOpt[orderNum].sectionName);
    memcpy(g_pkgCfgOpt[orderNum].sectionName, curNode->name, len);
    curNode = curNode->xmlChildrenNode;
    while (curNode != NULL) {
        if (xmlStrcmp(curNode->name, BAD_CAST"addrf") == 0) {
            key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
            value = atoi(key);
            g_pkgCfgOpt[orderNum].addrf = value;
            xmlFree(key);
        }

        if (xmlStrcmp( curNode->name, BAD_CAST"addrb") == 0) {
            key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
            value = atoi(key);
            g_pkgCfgOpt[orderNum].addrb = value;
            xmlFree(key);
        }

        if (xmlStrcmp( curNode->name, BAD_CAST"partsize") == 0) {
            key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
            value = atoi(key);
            g_pkgCfgOpt[orderNum].partSize = value;
            xmlFree(key);
        }

        if (xmlStrcmp(curNode->name, BAD_CAST"method") == 0) {
            key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
            if (strcmp(key, "diff") == 0) {
                g_pkgCfgOpt[orderNum].method = 1;
            }
            xmlFree(key);
        }

        if (xmlStrcmp( curNode->name, BAD_CAST"compress") == 0) {
            key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
            if (strcmp(key, "lzma") == 0) {
                g_pkgCfgOpt[orderNum].compress = 1;
            }
            xmlFree(key);
        }

        if (xmlStrcmp( curNode->name, BAD_CAST"oldname") == 0) {
            key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
            if (key != NULL) {
                len = sizeof(g_pkgCfgOpt[orderNum].oldFirmName) > strlen(key) ? strlen(key) : sizeof(g_pkgCfgOpt[orderNum].oldFirmName);
                memcpy(g_pkgCfgOpt[orderNum].oldFirmName, key, len);
                xmlFree(key);
            }
        }

        if (xmlStrcmp( curNode->name, BAD_CAST"newname") == 0) {
            key = xmlNodeListGetString(doc, curNode->xmlChildrenNode, 1);
            len = sizeof(g_pkgCfgOpt[orderNum].newFirmName) > strlen(key) ? strlen(key) : sizeof(g_pkgCfgOpt[orderNum].newFirmName);
            memcpy(g_pkgCfgOpt[orderNum].newFirmName, key, len);
            xmlFree(key);
        }

        curNode = curNode->next; 
    }

    return SwCfgUpdateElement(&g_pkgCfgOpt[orderNum]);
}

int SwCfgParseFile(const char *xmlFile)
{
    xmlDocPtr doc;
    xmlNodePtr curNode;

    doc = xmlParseFile(xmlFile);
    if (doc == NULL) {
        PARSE_ERR("Document[%s] not parsed successfully\n", xmlFile);
        return -1;
    }

    curNode = xmlDocGetRootElement(doc);
    if (curNode == NULL) {
        PARSE_ERR("empty document %s\n", xmlFile); 
        xmlFreeDoc(doc);
        return -1; 
    }

    if (xmlStrcmp(curNode->name, BAD_CAST"root") != 0) {
        PARSE_ERR("document of the wrong type, root node != root\n"); 
        xmlFreeDoc(doc);
        return -1; 
    }

    curNode = curNode->xmlChildrenNode;

    while (curNode != NULL) {
        if (xmlStrcmp(curNode->name, BAD_CAST"text") == 0) {
            curNode = curNode->next;
            continue;
        }
        if (SwCfgParseElement(doc, curNode) != 0) {
            xmlFreeDoc(doc);
            return -1;
        }
        curNode = curNode->next;
    }

    xmlFreeDoc(doc);
    return 0;
}

void SwCfgParseDumpHeader(PkgHeader *header)
{
    unsigned char debug[128];
    
    memset(debug, 0, sizeof(debug));
    memcpy(debug, header->magic, sizeof(header->magic));
    printf("------------pkg head dump--------------------\n");
    printf("magic:%s\n", debug);
    printf("patchver:%d\n", header->ver);
    printf("pkgsize:%d\n", header->pkgSize);
    printf("crc32:%08x\n", header->crc);
    memset(debug, 0, sizeof(debug));
    printf("sectionnum:%d\n", header->sectionNum);
    memcpy(debug, header->srcVersion, sizeof(header->srcVersion));
    printf("srcVersion:%s\n", debug);
    memset(debug, 0, sizeof(debug));
    memcpy(debug, header->dstVersion, sizeof(header->dstVersion));
    printf("dstVersion:%s\n", debug);
    printf("------------pkg head done--------------------\n");
}

void SwCfgParseDumpPartition(SectionOpt *part)
{
    printf("------------part dump--------------------\n");
    printf("sectionname:%s\n", part->sectionName);
    printf("partsize:%d\n", part->partSize);
    printf("sectionnum:%d\n", part->sectionNum);
    printf("addrf:%d\n", part->addrf);
    printf("addrb:%d\n", part->addrb);
    printf("addrflen:%d\n", part->addrfLen);
    printf("addrblen:%d\n", part->addrbLen);
    if (part->method == 1)
        printf("method:diff\n");
    else
        printf("method:total\n");
    if (part->compress == 1)
        printf("compress:lzma\n");
    else
        printf("compress:no\n");
    printf("oldfirmware:%s\n", part->oldFirmName);
    printf("newFirmName:%s\n", part->newFirmName);
    printf("------------part done--------------------\n");
}

void SwCfgParseDumpSlice(SectionOpt *part)
{
    int i, j;

    for (i = 0; i < part->sectionNum; i++) {
        printf("------------part slice[%d]-------------------\n", i);
        printf("sectionname:%s\n", part->sectionTable[i].sectionName);
        printf("addrf:%d\n", part->sectionTable[i].addrf);
        printf("addrfLen:%d\n", part->sectionTable[i].addrfLen);
        printf("addrb:%d\n", part->sectionTable[i].addrb);
        printf("addrbLen:%d\n", part->sectionTable[i].addrbLen);
        printf("patchoffset:%d\n", part->sectionTable[i].patchOffset);
        printf("patchsize:%d\n", part->sectionTable[i].patchSize);
        if (part->sectionTable[i].method == 1)
            printf("method:diff\n");
        else
            printf("method:total\n");
        if (part->sectionTable[i].compress)
            printf("compress:lzma\n");
        else
            printf("compress:no\n");
        printf("hashf:");
        for (j = 0; j < 32; j++)
            printf("%02x", part->sectionTable[i].hashf[j]);
        printf("\n");
        printf("hashb:");
        for (j = 0; j < 32; j++)
            printf("%02x", part->sectionTable[i].hashb[j]);
        printf("\n");
        printf("------------part done--------------------\n");
    }
}


int SwCfgParseInit()
{
    g_pkgCfgHeader = SwPkgHeaderGet();
    g_pkgCfgOpt = SwPkgSectionTableGet();

    return SwCfgParseFile(SW_PKG_CFG_FILENAME);
}
