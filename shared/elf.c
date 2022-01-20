/*
 * (C) 2022, Cornell University
 * All rights reserved.
 */

/* Author: Yunhao Zhang
 * Description: load an ELF file into memory; only using the single 
 * program header and not using the multiple section headers
 */

#include "elf.h"
#include "log.h"
#include <string.h>

void elf_load(struct block_store* bs) {
    char buf[512];
    bs->read(0, 1, buf);

    struct elf32_header *header = (void*) buf;
    INFO("ELF program header table entry count: %d", header->e_phnum);

    if (header->e_phnum != 1) {
        FATAL("Grass exec region of the disk seems to be corrupted");
    }
    
    if (header->e_phoff + header->e_phentsize > BLOCK_SIZE)  {
        FATAL("TODO: program header not in the first block of ELF");
    }
    
    struct elf32_program_header pheader;
    memcpy(&pheader, buf + header->e_phoff, sizeof(pheader));

    if (pheader.p_vaddr == GRASS_BASE) {
        /* load the grass kernel */
        INFO("Grass kernel starts at vaddr: 0x%.8x", pheader.p_vaddr);
        INFO("Grass kernel file offset: 0x%.8x", pheader.p_offset);
        INFO("Grass kernel memory size: 0x%.8x bytes", pheader.p_memsz);

        int block_offset = pheader.p_offset / BLOCK_SIZE;
        bs->read(block_offset, 1, buf);
        int size = BLOCK_SIZE - (pheader.p_offset % BLOCK_SIZE);
        memcpy((char*)GRASS_BASE, buf + (BLOCK_SIZE) - size, size);
        
        for (; size < pheader.p_filesz; size += BLOCK_SIZE)
            bs->read(++block_offset, 1, (char*)GRASS_BASE + size);

        memset((char*)GRASS_BASE + pheader.p_filesz, 0, GRASS_SIZE - pheader.p_filesz);

        /* call the grass kernel entry and never return */
        void (*grass_entry)() = (void*)GRASS_BASE;
        grass_entry();
    } else {
        FATAL("ELF gives invalid starting vaddr: 0x%.8x", pheader.p_vaddr);
    }
}
