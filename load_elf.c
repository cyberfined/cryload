#include "load_elf.h"
#include "lib.h"

#define PAGE_ALIGN  (1<<12)
#define ROUNDUP(x,b) (((unsigned long)x+b-1)&(~(b-1)))
#define ROUNDDOWN(x,b) ((unsigned long)x&(~(b-1)))
#define PAGEUP(x) ROUNDUP(x, PAGE_ALIGN)
#define PAGEDOWN(x) ROUNDDOWN(x, PAGE_ALIGN)

static inline void* elf_base_addr(void *rsp) {
    void *base_addr = NULL;
    unsigned long argc = *(unsigned long*)rsp;
    char **envp = rsp + (argc+2)*sizeof(unsigned long);
    while(*envp++);
    Elf64_auxv_t *aux = (Elf64_auxv_t*)envp;

    for(; aux->a_type != AT_NULL; aux++) {
        if(aux->a_type == AT_PHDR) {
            base_addr = (void*)(aux->a_un.a_val - sizeof(Elf64_Ehdr));
            break;
        }
    }

    return base_addr;
}

static inline size_t elf_memory_size(void *mapped) {
    Elf64_Ehdr *ehdr = mapped;
    Elf64_Phdr *phdr = mapped + ehdr->e_phoff;
    size_t mem_size = 0, segment_len;

    for(size_t i = 0; i < ehdr->e_phnum; i++, phdr++) {
        if(phdr->p_type != PT_LOAD)
            continue;
        segment_len = phdr->p_memsz + (phdr->p_vaddr & 0xfff);
        mem_size += PAGEUP(segment_len);
    }

    return mem_size;
}

void* elf_load_addr(void *rsp, void *mapped, size_t mapped_size) {
    Elf64_Ehdr *ehdr = mapped;
    if(ehdr->e_type == ET_EXEC)
        return NULL;

    size_t mem_size = elf_memory_size(mapped) + 0x1000;
    void *load_addr = elf_base_addr(rsp);

    if(mapped < load_addr && mapped + mapped_size > load_addr - mem_size)
        load_addr = mapped;

    return load_addr - mem_size;
}

void* load_elf(void *load_addr, void *mapped) {
    Elf64_Ehdr *ehdr = mapped;
    Elf64_Phdr *phdr = mapped + ehdr->e_phoff;
    void *text_segment = NULL;
    unsigned long initial_vaddr = 0;
    unsigned long brk_addr = 0;

    for(size_t i = 0; i < ehdr->e_phnum; i++, phdr++) {
        unsigned long rounded_len, k;
        void *segment;

        if(phdr->p_type != PT_LOAD)
            continue;

        if(text_segment != 0 && ehdr->e_type == ET_DYN) {
            load_addr = text_segment + phdr->p_vaddr - initial_vaddr;
            load_addr = (void*)PAGEDOWN(load_addr);
        } else if(ehdr->e_type == ET_EXEC) {
            load_addr = (void*)PAGEDOWN(phdr->p_vaddr);
        } 

        rounded_len = phdr->p_memsz + (phdr->p_vaddr & 0xfff);
        rounded_len = PAGEUP(rounded_len);

        segment = mmap(load_addr, rounded_len, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

        if(segment == MAP_FAILED)
            return NULL;

        if(ehdr->e_type == ET_EXEC)
            load_addr = (void*)phdr->p_vaddr;
        else
            load_addr = segment + (phdr->p_vaddr & 0xfff);
        memcpy(load_addr, mapped + phdr->p_offset, phdr->p_filesz);

        if(!text_segment) {
            text_segment = segment;
            initial_vaddr = phdr->p_vaddr;
        }

        unsigned int protflags = 0;
        if(phdr->p_flags & PF_R)
            protflags |= PROT_READ;
        if(phdr->p_flags & PF_W)
            protflags |= PROT_WRITE;
        if(phdr->p_flags & PF_X)
            protflags |= PROT_EXEC;

        mprotect(segment, rounded_len, protflags);

        k = phdr->p_vaddr + phdr->p_memsz;
        if(k > brk_addr) brk_addr = k;
    }

    if (ehdr->e_type == ET_EXEC) {
        brk(PAGEUP(brk_addr));
        load_addr = (void*)ehdr->e_entry;
    } else {
        load_addr = (void*)ehdr + ehdr->e_entry;
    }

    return load_addr;
}
