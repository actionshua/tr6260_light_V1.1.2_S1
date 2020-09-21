#ifndef __CONFIG_PARSE_H__
#define __CONFIG_PARSE_H__

void SwCfgParseDumpHeader(PkgHeader *header);
void SwCfgParseDumpPartition(SectionOpt *part);
void SwCfgParseDumpSlice(SectionOpt *part);
int SwCfgParseInit();

#endif