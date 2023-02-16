; basic example to demonstrate timer and GPIO
; this program just toggles the GPIO state periodically

.section data				; data section
$gpiomem:					; start of GPIO memory
	dw #0x90000000
$timermem:					; start of timer memory
	dw #0x90000080

.section text
$start:
	movi sp, #0x1000		; move stack pointer to 0x1000
	fw $gpiomem				; fetches $gpiomem address to r0
	lw r3, r0				; r3 = memory[r0] - fetches GPIO memory address to r3
	
	movi r10, #0			; status register for pin state

	movi r1, #1				; set pin 0 to output
	sw r1, r3				; write to GPIO Mode_0 register

	movi r1, #16			; IVT entry for IRQ
	fw $irqhandler			; fetch the IRQ handler routine address to r0
	sw r0, r1				; set the IRQ handler
	
	fw $timermem			; fetches the $timermem address to r0
	lw r4, r0				; r4 = memory[r0] - fetches timer memory address to r4
	addi r4, #24			; moves pointer to Compare_0
	movi r1, #1000			; set timer period to 1000 ticks
	sw r1, r4				; write to timer memory
	lw r4, r0				; moves back to Control timer register
	movi r1, #1				; build the timer control register contents
	sli r1, #8
	ori r1, #1
	sli r1, #16
	ori r1, #1
	sw r1, r4				; set the Control timer register - enable IRQ on compare 0, reset on compare 0, enable channel 0
	
$hang:
	bi $hang				; hang indefinitely
	
$irqhandler:
	push r0					; save registers to not overwrite outer context
	push r3
	push r4
	push r9
	
	fw $timermem			; fetches the $timermem address to r0
	lw r4, r0				; r4 <- memory[r0]
	addi r4, #4				; move to timer Status register
	lw r3, r4				; get status register
	andi r3, #1				; mask out everything except last bit (timer event_compare flag)
	cmpi r3, #1				; is the event_compare set?
	bi.ne $irqhandler_exit	; no? leave
	movi r3, #0				; clear IRQ flags
	sw r3, r4				; set status register
	
	fw $gpiomem				; fetches $gpiomem address to r0
	lw r3, r0				; r3 = memory[r0] - fetches GPIO memory address to r3
	movi r9, #1				; prepare the value to be set to the register (1 << 0) to set/clear GPIO 0 output
	cmpi r10, #1			; is the r10 flag set to 1?
	bi.eq $toggle_off		; yes - clear it below
	movi r10, #1			; set r10 flag to 1
	addi r3, #24			; Set_0
	bi $toggle
$toggle_off:
	movi r10, #0			; set r10 flag to 0
	addi r3, #32			; Clear_0
$toggle:
	sw r9, r3				; write to GPIO register (Set or Clear) to toggle state

$irqhandler_exit:
	pop r9					; restore registers
	pop r4
	pop r3
	pop r0
	mov pc, ra				; return back to outer context
