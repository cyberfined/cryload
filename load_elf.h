#ifndef LOAD_ELF_H
#define LOAD_ELF_H

#include "lib.h"
#include <elf.h>

void* elf_load_addr(void *rsp, void *mapped, size_t mapped_size);
void* load_elf(void *load_addr, void *mapped);

#endif
