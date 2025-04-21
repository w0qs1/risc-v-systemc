.global start

.text
# This is the vector table
# first location is for the reset handler
# second is the SysTick handler
# third is for external interrupt handler
start:
	jal x0, reset_handler
	jal x0, isr0
	jal x0, isr1

# SysTick interrupt handler
isr0:
	addi t2, zero, 1
	addi t1, zero, 512
	addi t6, zero, -1
	
	# Turn on Display and Temperature Sensor
	sw t6, 24(t1)
	
	# Enable Timer match interrupt again as it is one-shot timer
	sw t2, 32(t1)
	
	# return from isr (clears the flag)
	uret

# External interrupt handler (used as a placeholder
isr1:
	addi a1, a1, 1
	uret

# Application starts here
reset_handler:
	# Load the base address of peripherals
	addi t1, zero, 512
	
	# Get all 1s in t2 for setting direction as output
	addi t2, zero, -1
	# Set DDR1 to all 1s for output
	sw t2, 20(t1)
	
	# Enable SysTick timer match interrupt in CSR
	addi t2, zero, 1
	sw t2, 32(t1)
	
	# Set value of the Match register
	addi t2, zero, 32
	sw t2, 40(t1)

main:
	# Wait for interrupt
	wfi
	
	# Turn off Display and Temperature Sensro
	sw zero, 24(t1)
	
	# Repeat
	beq x0, x0, main
