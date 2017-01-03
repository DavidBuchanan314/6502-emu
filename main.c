#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>

#include "6502.h"
#include "6850.h"

struct termios oldTermios;

void step_delay()
{
	struct timespec req, rem;
	
	req.tv_sec = 0;
	req.tv_nsec = STEP_DURATION;
	
	nanosleep(&req, &rem);
}

void run_cpu()
{
	for (;;) {		
		// CPU timing is currently very far from being cycle-accurate
		for (int i = 0; i < (CPU_FREQ / (ONE_SECOND / STEP_DURATION)); i++) {
			step_cpu();
			step_uart();
		}
		step_delay(); // remove this for more speed
	}
}

void restore_stdin()
{
	tcsetattr(0, TCSANOW, &oldTermios);
}

void raw_stdin()
{
	struct termios newTermios;
	
	tcgetattr(0, &oldTermios);
	newTermios = oldTermios;
	cfmakeraw(&newTermios);
	tcsetattr(0, TCSANOW, &newTermios);
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
