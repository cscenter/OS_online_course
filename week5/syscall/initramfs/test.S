	.text
	.code64
	.global _start
_start:
	int	$0x80
	jmp	_start
