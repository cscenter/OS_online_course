	.code16
	.text
	.global start
start:
	/* We know that current physical address is 0x7c00,
	 * but there are different ways to repsent that
	 * address as segment and offset, and since gcc compiler
	 * dosen't know anything about segments and generates
	 * code assuming zero segment values we need to zero
	 * them manually. */
	ljmp $0x0, $real_start

real_start:
	movw $0, %ax
	movw %ax, %ds
	movw %ax, %ss

	/* call instruction requires stack initialized,
	 * so we use offset at 0x400 bytes above entry
	 * point. */
	movw $0x7c00, %sp
	addw $0x0400, %sp

	/* Video buffer is expected to start at address
	 * 0xb8000 so we need to split it in segment
	 * and offset, we use segment 0xb800 and offset
	 * starting from zero. */
	movw $0xB800, %ax
	movw %ax, %es
	movw $data, %si
	movw $0, %di
	movw size, %cx
	call memcpy

loop:
	jmp loop

/* copies %cx bytes from %ds:%si, %es:%di */
memcpy:
	testw %cx, %cx
	jz out
again:
	movb (%si), %ah
	movb %ah, %es:(%di)
	incw %si
	incw %di
	decw %cx
	jnz again
out:
	ret

data:
	/* First byte is code point and second byte is
	 * attribute (text color, background color, blink,
	 * underline). */
	.asciz "H\017e\017l\017l\017o\017!\017"
size:
	.short . - data
