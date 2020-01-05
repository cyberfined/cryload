# cryload

cryload was created for [cryptor](https://github.com/cyberfined/cryptor), program to compress and encrypt x86_64 elf files. cryptor create new elf file from given, its structure is:

| Elf headers                        |
|------------------------------------|
| Cryload                            |
|  Compressed and encrypted elf file |
| 256 byte of random data            |

Cryload is loader for encrypted and compressed x86_64 elf files. Loader finds compressed and encrypted elf file, then decrypt and decompress it. After that loader puts elf file in the right place in memory and jump to its entry point. Buffer at the end is used for aes key and iv initialization with linear congruential generator. For compression huffman encoding is used. For encryption aes is used.

# AES implementation

I use kokke's [tiny-AES-c](https://github.com/kokke/tiny-AES-c) implementation.

# load_elf

I use bediger's [load_elf](https://github.com/bediger4000/userlandexec) implementation.

# syscalls implementation

I use [musl-libc](https://www.musl-libc.org/) syscalls implementation.