ENTRY(_start)
SECTIONS {
    . = 0;
    .text :{
        *(.text) *(.text.startup) *(.data) *(.rodata)
        payload_size = .;
        QUAD(0)
        key_seed = .;
        QUAD(0)
        iv_seed = .;
        QUAD(0)
        loader_end = .;
    }
}

