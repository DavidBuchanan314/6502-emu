#include <stdio.h>
#include <unistd.h>
#include <sys/poll.h>
#include <stdlib.h>

#include "6502.h"
#include "6850.h"

int n;

void init_uart(CPU *cpu) {
	cpu->memory[DATA_ADDR] = 0;
	
	uart_SR.byte = 0;
	uart_SR.bits.TDRE = 1; // we are always ready to output data
	
	uart_SR.bits.RDRF = 0;
	incoming_char = 0;
	
}

int stdin_ready() {
	struct pollfd fds;
	fds.fd = 0; // stdin
	fds.events = POLLIN;
	return poll(&fds, 1, 0) == 1; // timeout = 0
}

void step_uart(CPU *cpu) {
	if (cpu->write_addr == &cpu->memory[DATA_ADDR]) {
		putchar(cpu->memory[DATA_ADDR]);
		if (cpu->memory[DATA_ADDR] == '\b') printf(" \b");
		fflush(stdout);
		cpu->write_addr = NULL;
	} else if (cpu->read_addr == &cpu->memory[DATA_ADDR]) {
		uart_SR.bits.RDRF = 0;
		cpu->read_addr = NULL;
	}
	
	/* update input register if empty */
	if ((n++ % 10000) == 0) { // polling stdin every cycle is performance intensive. This is a bit of a dirty hack.
		if (!uart_SR.bits.RDRF && stdin_ready()) { // the real hardware has no buffer. Remote the RDRF check for more accurate emulation.
			if (read(0, &incoming_char, 1) != 1) {
				printf("Warning: read() returned 0\n");
			}
			if (incoming_char == 0x18) { // CTRL+X
				printf("\r\n");
				exit(0);
			}
			if (incoming_char == 0x7F) { // Backspace
				incoming_char = '\b';
			}
			uart_SR.bits.RDRF = 1;
		}
	}
	
	cpu->memory[DATA_ADDR] = incoming_char;
	cpu->memory[CTRL_ADDR] = uart_SR.byte;
}
