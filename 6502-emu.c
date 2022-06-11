#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>

#include "6502.h"
#include "6850.h"

struct termios initial_termios;

void step_delay()
{
	struct timespec req, rem;
	
	req.tv_sec = 0;
	req.tv_nsec = STEP_DURATION;
	
	nanosleep(&req, &rem);
}

void run_cpu(uint64_t cycle_stop, int verbose, int mem_dump, int break_pc, int fast)
{
	uint64_t cycles = 0;
	uint64_t cycles_per_step = (CPU_FREQ / (ONE_SECOND / STEP_DURATION));
	
	for (;;) {
		for (cycles %= cycles_per_step; cycles < cycles_per_step;) {
			if (mem_dump) save_memory(NULL);
			cycles += step_cpu(verbose);
			if ((cycle_stop > 0) && (total_cycles >= cycle_stop)) goto end;
			step_uart();

			if (break_pc >= 0 && PC == (uint16_t)break_pc) {
				fprintf(stderr, "break at %04x\n", break_pc);
				save_memory(NULL);
				goto end;
			}
		}
		if (!fast) step_delay();
	}
end:
	return;
}

void restore_stdin()
{
	tcsetattr(0, TCSANOW, &initial_termios);
}

void raw_stdin()
{
	struct termios new_termios;
	
	tcgetattr(0, &initial_termios);
	new_termios = initial_termios;
	cfmakeraw(&new_termios);
	tcsetattr(0, TCSANOW, &new_termios);
	atexit(restore_stdin);
}

int hextoint(char *str) {
	int val;

	if (*str == '$') str++;
	val = strtol(str, NULL, 16);
	return val;
}

void usage(char *argv[]) {
	fprintf(stderr, "Usage: %s [OPTIONS] FILE\n"
		"Simulate a NMOS 6502 processor\n"
		"\nOPTIONS:\n"
		"\n  CPU Initialization (specify all values in hex; $nn, 0xNN, etc.)\n"
		"	-a HEX	set A register (default 0)\n"
		"	-x HEX	set X register (default 0)\n"
		"	-y HEX	set Y register (default 0)\n"
		"	-s HEX	set stack pointer (default $ff)\n"
		"	-p HEX	set processor status register (default 0)\n"
		"	-r ADDR	set initial run address (default: use value at RST_VEC)\n"
		"\n  Emulator Control\n"
		"	-v	print CPU info at every step\n"
		"	-i	connect stdin/stdout to the emulator\n"
		"	-b ADDR	stop when PC reaches this address, write memory dump, and exit\n"
		"	-c NUM	exit after number of cycles (default: never)\n"
		"	-f	run as fast as possible; no delay loop\n"
		"\n  Memory Initialization\n"
		"	-l ADDR	load address for ROM file (default $c000)\n"
		"	FILE	binary file to load\n"
		, argv[0]);
}

int main(int argc, char *argv[])
{
	int a, x, y, sp, sr, pc, load_addr;
	int verbose, interactive, mem_dump, break_pc, fast;
	uint64_t cycles;
	int opt;

	verbose = 0;
	interactive = 0;
	mem_dump = 0;
	cycles = 0;
	load_addr = 0xC000;
	break_pc = -1;
	fast = 0;
	a = 0;
	x = 0;
	y = 0;
	sp = 0xFF;
	sr = 0;
	pc = -RST_VEC;  // negative implies indirect
	while ((opt = getopt(argc, argv, "hvimfa:b:x:y:r:p:s:g:c:l:")) != -1) {
		switch (opt) {
		case 'v':
			verbose = 1;
			break;
		case 'i':
			interactive = 1;
			break;
		case 'm':
			mem_dump = 1;
			break;
		case 'f':
			fast = 1;
			break;
		case 'b':
			break_pc = hextoint(optarg);
			break;
		case 'a':
			a = hextoint(optarg);
			break;
		case 'x':
			x = hextoint(optarg);
			break;
		case 'y':
			y = hextoint(optarg);
			break;
		case 's':
			sp = hextoint(optarg);
			break;
		case 'p':
			sr = hextoint(optarg);
			break;
		case 'r':
		case 'g':
			pc = hextoint(optarg);
			break;
		case 'c':
			cycles = atol(optarg);
			break;
		case 'l':
			load_addr = hextoint(optarg);
			break;
		case 'h':
		default: /* '?' */
			usage(argv);
			exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Error: expected binary file to load\n\n");
		usage(argv);
		exit(EXIT_FAILURE);
	}
	if (load_rom(argv[optind], load_addr) != 0) {
		printf("Error loading \"%s\".\n", argv[optind]);
		return EXIT_FAILURE;
	}
	
	if (interactive) {
		printf("*** Entering interactive mode. CTRL+X to exit ***\n\n");
		raw_stdin(); // allow individual keystrokes to be detected
	}
	
	//init_tables();
	init_uart(interactive);
	
	reset_cpu(a, x, y, sp, sr, pc);
	run_cpu(cycles, verbose, mem_dump, break_pc, fast);
	
	return EXIT_SUCCESS;
}
