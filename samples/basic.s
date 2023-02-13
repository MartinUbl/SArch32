; very basic example to demonstrate usage
; this program just performs some unimportant arithmetic and fills the screen with black-white dot pattern

.section data				; data section
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
$videomem:					; start of video memory
	dw #0xA0000000

.section text
$start:
	movi sp, #0x1000		; move stack pointer to 0x1000
	movi r1, #20			; r1 = 20
	movi r2, #30			; r2 = 30
	add r1, r2				; r1 = r1 + r2
	push r1					; pushes r1 to stack
$loop:
	subi r1, #5				; subtract 5 from r1
	cmpi r1, #40			; compare r1 to 40
	bir.gt #-12				; if greater, repeat

	pop r1					; restores old r1 from stack
	fw $videomem			; fetches $videomem address to r0
	lw r3, r0				; r3 = memory[r0] - fetches video memory address to r3
	movi r5, #7500			; r5 is iteration variable, set 7500 repetitions (300x200 display size, 8 pixels per byte)
	movi r4, #0xAA			; move 0xAA to r4
	sli r4, #8				; shift r4 left by 8 bits
	ori r4, #0xAA			; add 0xAA to lowest bits
	sli r4, #8				; shift r4 left by 8 bits
	ori r4, #0xAA			; add 0xAA to lowest bits
	sli r4, #8				; shift r4 left by 8 bits
	ori r4, #0xAA			; add 0xAA to lowest bits
$fillmem:
	sw r4, r3				; stores r4 to r3 (video memory pointer)
	addi r3, #4				; moves r3 to next word
	subi r5, #4				; subtracts 4 from r5 (number of video memory bytes)
	cmpi r5, #0				; did we reach the end?
	bi.gt $fillmem			; if not, repeat
$hang:
	bi $hang				; hang indefinitely
