#!/bin/bash
# sed '$a'\0'' server_root_cert.pem
nds32le-elf-objcopy --input-target binary --output-target elf32-nds32le --binary-architecture nds32 --rename-section .data=.rodata.embedded server_root_cert.pem server_root_cert.pem.txt.o
#mv ../lib/ server_root_cert.pem.txt.o 