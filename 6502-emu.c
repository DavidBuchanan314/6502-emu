#include <stdio.h>
#include <stdlib.h>
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

void run_cpu()
{
	int cycles = 0;
	int cycles_per_step = (CPU_FREQ / (ONE_SECOND / STEP_DURATION));
	
	for (;;) {
		for (cycles %= cycles_per_step; cycles < cycles_per_step;) {
			cycles += step_cpu();
			step_uart();
		}
		step_delay(); // remove this for more speed
	}
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

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s file.rom\n", argv[0]);
		printf("The first 16k of \"file.rom\" is loaded into the last 16k of memory.\n");
		return EXIT_FAILURE;
	}
	
	if (load_rom(argv[1]) != 0) {
		printf("Error loading \"%s\".\n", argv[1]);
		return EXIT_FAILURE;
	}
	
	raw_stdin(); // allow individual keystrokes to be detected
	
	init_tables();
	init_uart();
	
	reset_cpu();
	run_cpu();
	
	return EXIT_SUCCESS;
}
