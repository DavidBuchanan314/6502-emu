#include <stdint.h>
#include <stdbool.h>

#define CPU_FREQ 4e6 // 4Mhz
#define STEP_DURATION 10e6 // 10ms
#define ONE_SECOND 1e9
#define NUM_MODES 14

#define NMI_VEC 0xFFFA
#define RST_VEC 0xFFFC
#define IRQ_VEC 0xFFFE

extern uint8_t memory[1<<16];
extern uint8_t A;
extern uint8_t X;
extern uint8_t Y;
extern uint16_t PC;
extern uint8_t SP; // points to first empty stack location
extern uint8_t extra_cycles;
extern uint64_t total_cycles;

extern void *read_addr;
extern void *write_addr;

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

extern union StatusReg SR;

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
	const char *mnemonic;
	void (*function)();
	Mode mode;
	uint8_t cycles;
} Instruction;

//extern Instruction instructions[0x100];

//void init_tables();

void reset_cpu(int _a, int _x, int _y, int _sp, int _sr, int _pc);

int load_rom(char *filename, int load_addr);

int step_cpu(int verbose);

void save_memory(const char *filename);
