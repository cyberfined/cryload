#include "lib.h"
#include "aes.h"
#include "huff.h"
#include "load_elf.h"
#include <elf.h>

#define SET_STACK(sp) __asm__ __volatile__ ("movq %0, %%rsp"::"r"(sp))
#define JMP(addr) __asm__ __volatile__ ("jmp *%0"::"r"(addr))

extern void* loader_end;
extern size_t payload_size;
extern size_t key_seed;
extern size_t iv_seed;

int main(int argc, char **argv) {
    uint8_t *payload = (uint8_t*)&loader_end;
    uint8_t *entropy_buf = payload + payload_size;
    void *rsp = argv-1;

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, entropy_buf, key_seed, iv_seed);
    AES_CTR_xcrypt_buffer(&ctx, payload, payload_size);
    memset(&ctx, 0, sizeof(ctx));

    size_t decoded_payload_size;
    char *decoded_payload = huffman_decode((char*)payload, payload_size, &decoded_payload_size);
    if(!decoded_payload) {
        AES_init_ctx_iv(&ctx, entropy_buf, key_seed, iv_seed);
        AES_CTR_xcrypt_buffer(&ctx, payload, payload_size);
        memset(&ctx, 0, sizeof(ctx));
        return 1;
    }

    void *load_addr = elf_load_addr(rsp, decoded_payload, decoded_payload_size);
    load_addr = load_elf(load_addr, decoded_payload);
    memset(decoded_payload, 0, decoded_payload_size);
    munmap(decoded_payload, decoded_payload_size);

    AES_init_ctx_iv(&ctx, entropy_buf, key_seed, iv_seed);
    AES_CTR_xcrypt_buffer(&ctx, payload, payload_size);
    memset(&ctx, 0, sizeof(ctx));
    if(!load_addr) {
        return 1;
    }

    SET_STACK(rsp);
    JMP(load_addr);
}
