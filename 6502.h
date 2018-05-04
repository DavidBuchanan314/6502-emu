#include <stdint.h>
#include <stdbool.h>

#define CPU_FREQ 4e6 // 4Mhz
#define STEP_DURATION 10e6 // 10ms
#define ONE_SECOND 1e9
#define NUM_MODES 14

#define NMI_VEC 0xFFFA
#define RST_VEC 0xFFFC
#define IRQ_VEC 0xFFFE

struct StatusBits{
	bool carry:1; // bit 0
	bool zero:1;
	bool interrupt:1;
	bool decimal:1;
	bool brk:1; // "break" is a reserved word :(
	bool unused:1;
	bool overflow:1;
	bool sign:1;	// bit 7
};

union StatusReg { // this means we can access the status register as a byte, or as individual bits.
	struct StatusBits bits;
	uint8_t byte;
};

typedef struct {
	uint8_t memory[1<<16];
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	uint16_t PC;
	uint8_t SP; // points to first empty stack location
	uint8_t extra_cycles;
	uint64_t total_cycles;
	union StatusReg SR;

	void * read_addr;
	void * write_addr;
	int jumping; // used to check that we don't need to increment the PC after a jump
} CPU;

typedef enum {
	ACC,
	ABS,
	ABSX,
	ABSY,
	IMM,
	IMPL,
	IND,
	XIND,
	INDY,
	REL,
	ZP,
	ZPX,
	ZPY,
	JMP_IND_BUG,
} Mode;

typedef struct {
	char * mnemonic;
	void (*function)(CPU *);
	Mode mode;
	uint8_t cycles;
} Instruction;

Instruction instructions[0x100];

void init_tables();

void reset_cpu(CPU *cpu, int _a, int _x, int _y, int _sp, int _sr, int _pc);

int load_rom(CPU *cpu, char * filename, int load_addr);

int step_cpu(CPU *cpu, int verbose);

void save_memory(CPU *cpu, char * filename);

CPU * create_cpu();
