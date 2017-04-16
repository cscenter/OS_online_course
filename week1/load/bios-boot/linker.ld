OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i8086)
ENTRY(start)

SECTIONS {
	. = 0x7C00;
	.text : { *(.text) }
	. = 0x7DFE;
	/* BIOS determine MBR by special signature in
	 * the last 2 bytes of 512 byte long sector. */
	.signature : { SHORT(0xAA55) }
}
