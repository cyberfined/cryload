AS=as
CC=gcc
CFLAGS=-std=c11 -Wall -Wno-builtin-declaration-mismatch -fno-stack-protector -O2
LDFLAGS=-nostdlib -nostartfiles -nodefaultlibs -nolibc -T linker.ld -pie -Xlinker --no-dynamic-linker -O2
SRCS=main.c aes.c huff.c load_elf.c
OBJS=$(patsubst %.c, %.o, $(SRCS))

TARGET_ELF=elf
TARGET_BIN=loader

.PHONY: all clean
all: $(TARGET_BIN)
$(TARGET_BIN): $(TARGET_ELF) mkloader
	./mkloader $(TARGET_ELF) $(TARGET_BIN)
$(TARGET_ELF): $(OBJS) start.o
	$(CC) $(OBJS) start.o -o $(TARGET_ELF) $(LDFLAGS)
start.o: start.s
	$(AS) -O2 -c start.s -o start.o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
mkloader: mkloader.c
	$(CC) -O2 mkloader.c -o mkloader
clean:
	rm -f $(OBJS) start.o $(TARGET_ELF) $(TARGET_BIN) mkloader
