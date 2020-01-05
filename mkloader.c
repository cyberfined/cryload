#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <elf.h>

static inline int check_elf(void *mapped) {
    Elf64_Ehdr *ehdr = mapped;

    if(memcmp(ehdr->e_ident, ELFMAG, SELFMAG)) {
        fputs("check_elf: wrong elf magic", stderr);
        return -1;
    }

    if(ehdr->e_version != EV_CURRENT) {
        fputs("check_elf: wrong elf version", stderr);
        return -1;
    }

    return 0;
}

static inline Elf64_Shdr* section_by_name(void *mapped, const char *sh_name) {
    Elf64_Ehdr *ehdr = mapped;
    Elf64_Shdr *shdr = mapped + ehdr->e_shoff;
    char *shstrtab = mapped + shdr[ehdr->e_shstrndx].sh_offset;

    for(int i = 0; i < ehdr->e_shnum; i++, shdr++) {
        if(!strcmp(shstrtab + shdr->sh_name, sh_name))
            return shdr;
    }

    return NULL;
}

static inline Elf64_Shdr* section_by_type(void *mapped, Elf64_Word sh_type) {
    Elf64_Ehdr *ehdr = mapped;
    Elf64_Shdr *shdr = mapped + ehdr->e_shoff;

    for(int i = 0; i < ehdr->e_shnum; i++, shdr++) {
        if(shdr->sh_type == sh_type)
            return shdr;
    }

    return NULL;
}

static inline Elf64_Sym* symbol_by_name(void *mapped, const char *sym_name) {
    Elf64_Ehdr *ehdr = mapped;
    Elf64_Shdr *shdr = mapped + ehdr->e_shoff;
    Elf64_Shdr *shtsym = section_by_type(mapped, SHT_SYMTAB);
    if(!shtsym)
        return NULL;

    Elf64_Sym *sym = mapped + shtsym->sh_offset;
    char *symstrtab = mapped + shdr[shtsym->sh_link].sh_offset;
    int num_syms = shtsym->sh_size/sizeof(Elf64_Sym);

    for(int i = 0; i < num_syms; i++, sym++) {
        if(!strcmp(symstrtab + sym->st_name, sym_name))
            return sym;
    }

    return NULL;
}

static inline int write_section(void *mapped, int fd, const char *sh_name) {
    Elf64_Shdr *sh_text = section_by_name(mapped, sh_name);
    if(!sh_text) {
        fprintf(stderr, "write_text_section: elf doesn't contain %s section\n", sh_name);
        return -1;
    }

    if(write(fd, mapped + sh_text->sh_offset, sh_text->sh_size) != sh_text->sh_size) {
        perror("write");
        return -1;
    }

    return 0;
}

static inline int write_symbol_offset(void *mapped, int fd, const char *sym_name) {
    Elf64_Ehdr *ehdr = mapped;
    Elf64_Shdr *shdr = mapped + ehdr->e_shoff;
    Elf64_Sym *sym = symbol_by_name(mapped, sym_name);
    if(!sym) {
        fprintf(stderr, "write_symbol_offset: elf doesn't contain %s symbol\n", sym_name);
        return -1;
    }

    shdr += sym->st_shndx;
    size_t sym_off = sym->st_value - shdr->sh_addr;
    if(write(fd, &sym_off, sizeof(sym_off)) != sizeof(sym_off)) {
        perror("write");
        return -1;
    }

    return 0;
}

static inline int write_entry_offset(void *mapped, int fd) {
    Elf64_Ehdr *ehdr = mapped;
    Elf64_Shdr *shdr = section_by_name(mapped, ".text");
    if(!shdr)
        return -1;
    size_t offset = ehdr->e_entry - shdr->sh_addr;

    if(write(fd, &offset, sizeof(offset)) < 0) {
        perror("write");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    int in_fd = -1, out_fd = -1;
    struct stat in_stat;
    void *mapped = MAP_FAILED;
    size_t mapped_size;
    int ret = 1;

    if(argc != 3) {
        fprintf(stderr, "Usage: %s <elf> <output>\n", argv[0]);
        goto exit;
    }

    in_fd = open(argv[1], O_RDONLY);
    if(in_fd < 0) {
        perror("open");
        goto exit;
    }

    if(fstat(in_fd, &in_stat) < 0) {
        perror("fstat");
        goto exit;
    }
    mapped_size = in_stat.st_size;

    mapped = mmap(NULL, mapped_size, PROT_READ, MAP_PRIVATE, in_fd, 0);
    if(mapped == MAP_FAILED) {
        perror("mmap");
        goto exit;
    }

    out_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(out_fd < 0) {
        perror("open");
        goto exit;
    }

    if(check_elf(mapped) < 0)
        goto exit;

    if(write_entry_offset(mapped, out_fd) < 0)
        goto exit;

    if(write_symbol_offset(mapped, out_fd, "payload_size") < 0 ||
       write_symbol_offset(mapped, out_fd, "key_seed") < 0 ||
       write_symbol_offset(mapped, out_fd, "iv_seed") < 0)
    {
        goto exit;
    }

    if(write_section(mapped, out_fd, ".text") < 0)
        goto exit;

    ret = 0;
exit:
    if(in_fd >= 0) close(in_fd);
    if(out_fd >= 0) close(out_fd);
    if(mapped != MAP_FAILED) munmap(mapped, mapped_size);
    return ret;
}
