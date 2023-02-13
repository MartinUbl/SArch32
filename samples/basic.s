; very basic example to demonstrate usage

.section data
$test_byte:
	db #123
$test_word:
	dw #30640
$word_ptr:
	dw $test_word
$test_string:
	asciz 'Hello world'
$test_abc:
	db #111
$videomem:
	dw #0xA0000000

.section text
$start:
	movi sp, #1000
	movi r1, #20
	movi r2, #30
	add r1, r2
	push r1
$loop:
	subi r1, #5
	cmpi r1, #40
	bir.gt #-12
	pop r1
	;svc #12
	fw $videomem
	lw r3, r0
	movi r5, #7500
$fillmem:
	movi r4, #0xAA
	sli r4, #8
	addi r4, #0xAA
	sli r4, #8
	addi r4, #0xAA
	sli r4, #8
	addi r4, #0xAA
	sw r4, r3
	addi r3, #1
	subi r5, #1
	cmpi r5, #0
	bi.gt $fillmem
$hang:
	bi $hang
