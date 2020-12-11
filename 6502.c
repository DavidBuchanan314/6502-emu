#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "6502.h"

uint8_t memory[1<<16];
uint8_t A;
uint8_t X;
uint8_t Y;
uint16_t PC;
uint8_t SP;
uint8_t extra_cycles;
uint64_t total_cycles;
union StatusReg SR;

static uint8_t *get_ACC(void);
static uint8_t *get_ABS(void);
static uint8_t *get_ABSX(void);
static uint8_t *get_ABSY(void);
static uint8_t *get_IMM(void);
static uint8_t *get_IMPL(void);
static uint8_t *get_IND(void);
static uint8_t *get_XIND(void);
static uint8_t *get_INDY(void);
static uint8_t *get_REL(void);
static uint8_t *get_ZP(void);
static uint8_t *get_ZPX(void);
static uint8_t *get_ZPY(void);
static uint8_t *get_JMP_IND_BUG(void);

static void inst_ADC(void);
static void inst_AND(void);
static void inst_ASL(void);
static void inst_BCC(void);
static void inst_BCS(void);
static void inst_BEQ(void);
static void inst_BIT(void);
static void inst_BMI(void);
static void inst_BNE(void);
static void inst_BPL(void);
static void inst_BRK(void);
static void inst_BVC(void);
static void inst_BVS(void);
static void inst_CLC(void);
static void inst_CLD(void);
static void inst_CLI(void);
static void inst_CLV(void);
static void inst_CMP(void);
static void inst_CPX(void);
static void inst_CPY(void);
static void inst_DEC(void);
static void inst_DEX(void);
static void inst_DEY(void);
static void inst_EOR(void);
static void inst_INC(void);
static void inst_INX(void);
static void inst_INY(void);
static void inst_JMP(void);
static void inst_JSR(void);
static void inst_LDA(void);
static void inst_LDX(void);
static void inst_LDY(void);
static void inst_LSR(void);
static void inst_NOP(void);
static void inst_ORA(void);
static void inst_PHA(void);
static void inst_PHP(void);
static void inst_PLA(void);
static void inst_PLP(void);
static void inst_ROL(void);
static void inst_ROR(void);
static void inst_RTI(void);
static void inst_RTS(void);
static void inst_SBC(void);
static void inst_SEC(void);
static void inst_SED(void);
static void inst_SEI(void);
static void inst_STA(void);
static void inst_STX(void);
static void inst_STY(void);
static void inst_TAX(void);
static void inst_TAY(void);
static void inst_TSX(void);
static void inst_TXA(void);
static void inst_TXS(void);
static void inst_TYA(void);


// instruction length table, indexed by addressing mode
static int const lengths[NUM_MODES] = {
	[ACC]         = 1,
	[ABS]         = 3,
	[ABSX]        = 3,
	[ABSY]        = 3,
	[IMM]         = 2,
	[IMPL]        = 1,
	[IND]         = 3,
	[XIND]        = 2,
	[INDY]        = 2,
	[REL]         = 2,
	[ZP]          = 2,
	[ZPX]         = 2,
	[ZPY]         = 2,
	[JMP_IND_BUG] = 3
};

// addressing mode decoder table
static uint8_t *(*const get_ptr[NUM_MODES])() = {
	[ACC]         = get_ACC,
	[ABS]         = get_ABS,
	[ABSX]        = get_ABSX,
	[ABSY]        = get_ABSY,
	[IMM]         = get_IMM,
	[IMPL]        = get_IMPL,
	[IND]         = get_IND,
	[XIND]        = get_XIND,
	[INDY]        = get_INDY,
	[REL]         = get_REL,
	[ZP]          = get_ZP,
	[ZPX]         = get_ZPX,
	[ZPY]         = get_ZPY,
	[JMP_IND_BUG] = get_JMP_IND_BUG
};

// instruction data table
static const Instruction instructions[0x100] = {
	[0x00] = {"BRK impl", inst_BRK, IMPL, 7},
	[0x01] = {"ORA X,ind", inst_ORA, XIND, 6},
	[0x02] = {"???", inst_NOP, IMPL, 2},
	[0x03] = {"???", inst_NOP, IMPL, 8},
	[0x04] = {"???", inst_NOP, ZP, 3},
	[0x05] = {"ORA zpg", inst_ORA, ZP, 3},
	[0x06] = {"ASL zpg", inst_ASL, ZP, 5},
	[0x07] = {"???", inst_NOP, IMPL, 5},
	[0x08] = {"PHP impl", inst_PHP, IMPL, 3},
	[0x09] = {"ORA #", inst_ORA, IMM, 2},
	[0x0A] = {"ASL A", inst_ASL, ACC, 2},
	[0x0B] = {"???", inst_NOP, IMPL, 2},
	[0x0C] = {"???", inst_NOP, ABS, 4},
	[0x0D] = {"ORA abs", inst_ORA, ABS, 4},
	[0x0E] = {"ASL abs", inst_ASL, ABS, 6},
	[0x0F] = {"???", inst_NOP, IMPL, 6},
	[0x10] = {"BPL rel", inst_BPL, REL, 2},
	[0x11] = {"ORA ind,Y", inst_ORA, INDY, 5},
	[0x12] = {"???", inst_NOP, IMPL, 2},
	[0x13] = {"???", inst_NOP, IMPL, 8},
	[0x14] = {"???", inst_NOP, ZP, 4},
	[0x15] = {"ORA zpg,X", inst_ORA, ZPX, 4},
	[0x16] = {"ASL zpg,X", inst_ASL, ZPX, 6},
	[0x17] = {"???", inst_NOP, IMPL, 6},
	[0x18] = {"CLC impl", inst_CLC, IMPL, 2},
	[0x19] = {"ORA abs,Y", inst_ORA, ABSY, 4},
	[0x1A] = {"???", inst_NOP, IMPL, 2},
	[0x1B] = {"???", inst_NOP, IMPL, 7},
	[0x1C] = {"???", inst_NOP, ABSX, 4},
	[0x1D] = {"ORA abs,X", inst_ORA, ABSX, 4},
	[0x1E] = {"ASL abs,X", inst_ASL, ABSX, 7},
	[0x1F] = {"???", inst_NOP, IMPL, 7},
	[0x20] = {"JSR abs", inst_JSR, ABS, 6},
	[0x21] = {"AND X,ind", inst_AND, XIND, 6},
	[0x22] = {"???", inst_NOP, IMPL, 2},
	[0x23] = {"???", inst_NOP, IMPL, 8},
	[0x24] = {"BIT zpg", inst_BIT, ZP, 3},
	[0x25] = {"AND zpg", inst_AND, ZP, 3},
	[0x26] = {"ROL zpg", inst_ROL, ZP, 5},
	[0x27] = {"???", inst_NOP, IMPL, 5},
	[0x28] = {"PLP impl", inst_PLP, IMPL, 4},
	[0x29] = {"AND #", inst_AND, IMM, 2},
	[0x2A] = {"ROL A", inst_ROL, ACC, 2},
	[0x2B] = {"???", inst_NOP, IMPL, 2},
	[0x2C] = {"BIT abs", inst_BIT, ABS, 4},
	[0x2D] = {"AND abs", inst_AND, ABS, 4},
	[0x2E] = {"ROL abs", inst_ROL, ABS, 6},
	[0x2F] = {"???", inst_NOP, IMPL, 6},
	[0x30] = {"BMI rel", inst_BMI, REL, 2},
	[0x31] = {"AND ind,Y", inst_AND, INDY, 5},
	[0x32] = {"???", inst_NOP, IMPL, 2},
	[0x33] = {"???", inst_NOP, IMPL, 8},
	[0x34] = {"???", inst_NOP, ZP, 4},
	[0x35] = {"AND zpg,X", inst_AND, ZPX, 4},
	[0x36] = {"ROL zpg,X", inst_ROL, ZPX, 6},
	[0x37] = {"???", inst_NOP, IMPL, 6},
	[0x38] = {"SEC impl", inst_SEC, IMPL, 2},
	[0x39] = {"AND abs,Y", inst_AND, ABSY, 4},
	[0x3A] = {"???", inst_NOP, IMPL, 2},
	[0x3B] = {"???", inst_NOP, IMPL, 7},
	[0x3C] = {"???", inst_NOP, ABSX, 4},
	[0x3D] = {"AND abs,X", inst_AND, ABSX, 4},
	[0x3E] = {"ROL abs,X", inst_ROL, ABSX, 7},
	[0x3F] = {"???", inst_NOP, IMPL, 7},
	[0x40] = {"RTI impl", inst_RTI, IMPL, 6},
	[0x41] = {"EOR X,ind", inst_EOR, XIND, 6},
	[0x42] = {"???", inst_NOP, IMPL, 2},
	[0x43] = {"???", inst_NOP, IMPL, 8},
	[0x44] = {"???", inst_NOP, ZP, 3},
	[0x45] = {"EOR zpg", inst_EOR, ZP, 3},
	[0x46] = {"LSR zpg", inst_LSR, ZP, 5},
	[0x47] = {"???", inst_NOP, IMPL, 5},
	[0x48] = {"PHA impl", inst_PHA, IMPL, 3},
	[0x49] = {"EOR #", inst_EOR, IMM, 2},
	[0x4A] = {"LSR A", inst_LSR, ACC, 2},
	[0x4B] = {"???", inst_NOP, IMPL, 2},
	[0x4C] = {"JMP abs", inst_JMP, ABS, 3},
	[0x4D] = {"EOR abs", inst_EOR, ABS, 4},
	[0x4E] = {"LSR abs", inst_LSR, ABS, 6},
	[0x4F] = {"???", inst_NOP, IMPL, 6},
	[0x50] = {"BVC rel", inst_BVC, REL, 2},
	[0x51] = {"EOR ind,Y", inst_EOR, INDY, 5},
	[0x52] = {"???", inst_NOP, IMPL, 2},
	[0x53] = {"???", inst_NOP, IMPL, 8},
	[0x54] = {"???", inst_NOP, ZP, 4},
	[0x55] = {"EOR zpg,X", inst_EOR, ZPX, 4},
	[0x56] = {"LSR zpg,X", inst_LSR, ZPX, 6},
	[0x57] = {"???", inst_NOP, IMPL, 6},
	[0x58] = {"CLI impl", inst_CLI, IMPL, 2},
	[0x59] = {"EOR abs,Y", inst_EOR, ABSY, 4},
	[0x5A] = {"???", inst_NOP, IMPL, 2},
	[0x5B] = {"???", inst_NOP, IMPL, 7},
	[0x5C] = {"???", inst_NOP, ABSX, 4},
	[0x5D] = {"EOR abs,X", inst_EOR, ABSX, 4},
	[0x5E] = {"LSR abs,X", inst_LSR, ABSX, 7},
	[0x5F] = {"???", inst_NOP, IMPL, 7},
	[0x60] = {"RTS impl", inst_RTS, IMPL, 6},
	[0x61] = {"ADC X,ind", inst_ADC, XIND, 6},
	[0x62] = {"???", inst_NOP, IMPL, 2},
	[0x63] = {"???", inst_NOP, IMPL, 8},
	[0x64] = {"???", inst_NOP, ZP, 3},
	[0x65] = {"ADC zpg", inst_ADC, ZP, 3},
	[0x66] = {"ROR zpg", inst_ROR, ZP, 5},
	[0x67] = {"???", inst_NOP, IMPL, 5},
	[0x68] = {"PLA impl", inst_PLA, IMPL, 4},
	[0x69] = {"ADC #", inst_ADC, IMM, 2},
	[0x6A] = {"ROR A", inst_ROR, ACC, 2},
	[0x6B] = {"???", inst_NOP, IMPL, 2},
	[0x6C] = {"JMP ind", inst_JMP, JMP_IND_BUG, 5},
	[0x6D] = {"ADC abs", inst_ADC, ABS, 4},
	[0x6E] = {"ROR abs", inst_ROR, ABS, 6},
	[0x6F] = {"???", inst_NOP, IMPL, 6},
	[0x70] = {"BVS rel", inst_BVS, REL, 2},
	[0x71] = {"ADC ind,Y", inst_ADC, INDY, 5},
	[0x72] = {"???", inst_NOP, IMPL, 2},
	[0x73] = {"???", inst_NOP, IMPL, 8},
	[0x74] = {"???", inst_NOP, ZP, 4},
	[0x75] = {"ADC zpg,X", inst_ADC, ZPX, 4},
	[0x76] = {"ROR zpg,X", inst_ROR, ZPX, 6},
	[0x77] = {"???", inst_NOP, IMPL, 6},
	[0x78] = {"SEI impl", inst_SEI, IMPL, 2},
	[0x79] = {"ADC abs,Y", inst_ADC, ABSY, 4},
	[0x7A] = {"???", inst_NOP, IMPL, 2},
	[0x7B] = {"???", inst_NOP, IMPL, 7},
	[0x7C] = {"???", inst_NOP, ABSX, 4},
	[0x7D] = {"ADC abs,X", inst_ADC, ABSX, 4},
	[0x7E] = {"ROR abs,X", inst_ROR, ABSX, 7},
	[0x7F] = {"???", inst_NOP, IMPL, 7},
	[0x80] = {"???", inst_NOP, IMM, 2},
	[0x81] = {"STA X,ind", inst_STA, XIND, 6},
	[0x82] = {"???", inst_NOP, IMPL, 2},
	[0x83] = {"???", inst_NOP, IMPL, 6},
	[0x84] = {"STY zpg", inst_STY, ZP, 3},
	[0x85] = {"STA zpg", inst_STA, ZP, 3},
	[0x86] = {"STX zpg", inst_STX, ZP, 3},
	[0x87] = {"???", inst_NOP, IMPL, 3},
	[0x88] = {"DEY impl", inst_DEY, IMPL, 2},
	[0x89] = {"???", inst_NOP, IMPL, 2},
	[0x8A] = {"TXA impl", inst_TXA, IMPL, 2},
	[0x8B] = {"???", inst_NOP, IMPL, 2},
	[0x8C] = {"STY abs", inst_STY, ABS, 4},
	[0x8D] = {"STA abs", inst_STA, ABS, 4},
	[0x8E] = {"STX abs", inst_STX, ABS, 4},
	[0x8F] = {"???", inst_NOP, IMPL, 4},
	[0x90] = {"BCC rel", inst_BCC, REL, 2},
	[0x91] = {"STA ind,Y", inst_STA, INDY, 6},
	[0x92] = {"???", inst_NOP, IMPL, 2},
	[0x93] = {"???", inst_NOP, IMPL, 6},
	[0x94] = {"STY zpg,X", inst_STY, ZPX, 4},
	[0x95] = {"STA zpg,X", inst_STA, ZPX, 4},
	[0x96] = {"STX zpg,Y", inst_STX, ZPY, 4},
	[0x97] = {"???", inst_NOP, IMPL, 4},
	[0x98] = {"TYA impl", inst_TYA, IMPL, 2},
	[0x99] = {"STA abs,Y", inst_STA, ABSY, 5},
	[0x9A] = {"TXS impl", inst_TXS, IMPL, 2},
	[0x9B] = {"???", inst_NOP, IMPL, 5},
	[0x9C] = {"???", inst_NOP, IMPL, 5},
	[0x9D] = {"STA abs,X", inst_STA, ABSX, 5},
	[0x9E] = {"???", inst_NOP, IMPL, 5},
	[0x9F] = {"???", inst_NOP, IMPL, 5},
	[0xA0] = {"LDY #", inst_LDY, IMM, 2},
	[0xA1] = {"LDA X,ind", inst_LDA, XIND, 6},
	[0xA2] = {"LDX #", inst_LDX, IMM, 2},
	[0xA3] = {"???", inst_NOP, IMPL, 6},
	[0xA4] = {"LDY zpg", inst_LDY, ZP, 3},
	[0xA5] = {"LDA zpg", inst_LDA, ZP, 3},
	[0xA6] = {"LDX zpg", inst_LDX, ZP, 3},
	[0xA7] = {"???", inst_NOP, IMPL, 3},
	[0xA8] = {"TAY impl", inst_TAY, IMPL, 2},
	[0xA9] = {"LDA #", inst_LDA, IMM, 2},
	[0xAA] = {"TAX impl", inst_TAX, IMPL, 2},
	[0xAB] = {"???", inst_NOP, IMPL, 2},
	[0xAC] = {"LDY abs", inst_LDY, ABS, 4},
	[0xAD] = {"LDA abs", inst_LDA, ABS, 4},
	[0xAE] = {"LDX abs", inst_LDX, ABS, 4},
	[0xAF] = {"???", inst_NOP, IMPL, 4},
	[0xB0] = {"BCS rel", inst_BCS, REL, 2},
	[0xB1] = {"LDA ind,Y", inst_LDA, INDY, 5},
	[0xB2] = {"???", inst_NOP, IMPL, 2},
	[0xB3] = {"???", inst_NOP, IMPL, 5},
	[0xB4] = {"LDY zpg,X", inst_LDY, ZPX, 4},
	[0xB5] = {"LDA zpg,X", inst_LDA, ZPX, 4},
	[0xB6] = {"LDX zpg,Y", inst_LDX, ZPY, 4},
	[0xB7] = {"???", inst_NOP, IMPL, 4},
	[0xB8] = {"CLV impl", inst_CLV, IMPL, 2},
	[0xB9] = {"LDA abs,Y", inst_LDA, ABSY, 4},
	[0xBA] = {"TSX impl", inst_TSX, IMPL, 2},
	[0xBB] = {"???", inst_NOP, IMPL, 4},
	[0xBC] = {"LDY abs,X", inst_LDY, ABSX, 4},
	[0xBD] = {"LDA abs,X", inst_LDA, ABSX, 4},
	[0xBE] = {"LDX abs,Y", inst_LDX, ABSY, 4},
	[0xBF] = {"???", inst_NOP, IMPL, 4},
	[0xC0] = {"CPY #", inst_CPY, IMM, 2},
	[0xC1] = {"CMP X,ind", inst_CMP, XIND, 6},
	[0xC2] = {"???", inst_NOP, IMPL, 2},
	[0xC3] = {"???", inst_NOP, IMPL, 8},
	[0xC4] = {"CPY zpg", inst_CPY, ZP, 3},
	[0xC5] = {"CMP zpg", inst_CMP, ZP, 3},
	[0xC6] = {"DEC zpg", inst_DEC, ZP, 5},
	[0xC7] = {"???", inst_NOP, IMPL, 5},
	[0xC8] = {"INY impl", inst_INY, IMPL, 2},
	[0xC9] = {"CMP #", inst_CMP, IMM, 2},
	[0xCA] = {"DEX impl", inst_DEX, IMPL, 2},
	[0xCB] = {"???", inst_NOP, IMPL, 2},
	[0xCC] = {"CPY abs", inst_CPY, ABS, 4},
	[0xCD] = {"CMP abs", inst_CMP, ABS, 4},
	[0xCE] = {"DEC abs", inst_DEC, ABS, 6},
	[0xCF] = {"???", inst_NOP, IMPL, 6},
	[0xD0] = {"BNE rel", inst_BNE, REL, 2},
	[0xD1] = {"CMP ind,Y", inst_CMP, INDY, 5},
	[0xD2] = {"???", inst_NOP, IMPL, 2},
	[0xD3] = {"???", inst_NOP, IMPL, 8},
	[0xD4] = {"???", inst_NOP, ZP, 4},
	[0xD5] = {"CMP zpg,X", inst_CMP, ZPX, 4},
	[0xD6] = {"DEC zpg,X", inst_DEC, ZPX, 6},
	[0xD7] = {"???", inst_NOP, IMPL, 6},
	[0xD8] = {"CLD impl", inst_CLD, IMPL, 2},
	[0xD9] = {"CMP abs,Y", inst_CMP, ABSY, 4},
	[0xDA] = {"???", inst_NOP, IMPL, 2},
	[0xDB] = {"???", inst_NOP, IMPL, 7},
	[0xDC] = {"???", inst_NOP, ABSX, 4},
	[0xDD] = {"CMP abs,X", inst_CMP, ABSX, 4},
	[0xDE] = {"DEC abs,X", inst_DEC, ABSX, 7},
	[0xDF] = {"???", inst_NOP, IMPL, 7},
	[0xE0] = {"CPX #", inst_CPX, IMM, 2},
	[0xE1] = {"SBC X,ind", inst_SBC, XIND, 6},
	[0xE2] = {"???", inst_NOP, IMPL, 2},
	[0xE3] = {"???", inst_NOP, IMPL, 8},
	[0xE4] = {"CPX zpg", inst_CPX, ZP, 3},
	[0xE5] = {"SBC zpg", inst_SBC, ZP, 3},
	[0xE6] = {"INC zpg", inst_INC, ZP, 5},
	[0xE7] = {"???", inst_NOP, IMPL, 5},
	[0xE8] = {"INX impl", inst_INX, IMPL, 2},
	[0xE9] = {"SBC #", inst_SBC, IMM, 2},
	[0xEA] = {"NOP impl", inst_NOP, IMPL, 2},
	[0xEB] = {"???", inst_NOP, IMPL, 2},
	[0xEC] = {"CPX abs", inst_CPX, ABS, 4},
	[0xED] = {"SBC abs", inst_SBC, ABS, 4},
	[0xEE] = {"INC abs", inst_INC, ABS, 6},
	[0xEF] = {"???", inst_NOP, IMPL, 6},
	[0xF0] = {"BEQ rel", inst_BEQ, REL, 2},
	[0xF1] = {"SBC ind,Y", inst_SBC, INDY, 5},
	[0xF2] = {"???", inst_NOP, IMPL, 2},
	[0xF3] = {"???", inst_NOP, IMPL, 8},
	[0xF4] = {"???", inst_NOP, ZP, 4},
	[0xF5] = {"SBC zpg,X", inst_SBC, ZPX, 4},
	[0xF6] = {"INC zpg,X", inst_INC, ZPX, 6},
	[0xF7] = {"???", inst_NOP, IMPL, 6},
	[0xF8] = {"SED impl", inst_SED, IMPL, 2},
	[0xF9] = {"SBC abs,Y", inst_SBC, ABSY, 4},
	[0xFA] = {"???", inst_NOP, IMPL, 2},
	[0xFB] = {"???", inst_NOP, IMPL, 7},
	[0xFC] = {"???", inst_NOP, ABSX, 4},
	[0xFD] = {"SBC abs,X", inst_SBC, ABSX, 4},
	[0xFE] = {"INC abs,X", inst_INC, ABSX, 7},
	[0xFF] = {"???", inst_NOP, IMPL, 7}
};

Instruction inst; // the current instruction (used for convenience)
int jumping; // used to check that we don't need to increment the PC after a jump
void *read_addr;
void *write_addr;

/* Flag Checks */

static inline void N_flag(int8_t val)
{
	SR.bits.sign = val < 0;
}

static inline void Z_flag(uint8_t val)
{
	SR.bits.zero = val == 0;
}

/* Stack Helpers */

static inline void stack_push(uint8_t val)
{
	memory[0x100+(SP--)] = val;
}

static inline uint8_t stack_pull()
{
	return memory[0x100+(++SP)];
}

/* Memory read/write wrappers */

static inline uint8_t *read_ptr()
{
	return read_addr = get_ptr[inst.mode]();
}

static inline uint8_t *write_ptr()
{
	return write_addr = get_ptr[inst.mode]();
}

/* Branch logic common to all branch instructions */

static inline void take_branch()
{
	uint16_t oldPC;
	oldPC = PC + 2; // PC has already moved to point to the next instruction
	PC = read_ptr() - memory;
	if ((PC ^ oldPC) & 0xff00) extra_cycles += 1; // addr crosses page boundary
	extra_cycles += 1;
}

/* Instruction Implementations */

static void inst_ADC(void)
{
	uint8_t operand = *read_ptr();
	unsigned int tmp = A + operand + (SR.bits.carry & 1);
	if (SR.bits.decimal) {
		tmp = (A & 0x0f) + (operand & 0x0f) + (SR.bits.carry & 1);
		if (tmp >= 10) tmp = (tmp - 10) | 0x10;
		tmp += (A & 0xf0) + (operand & 0xf0);
		if (tmp > 0x9f) tmp += 0x60;
	}
	SR.bits.carry = tmp > 0xFF;
	SR.bits.overflow =  ((A^tmp)&(operand^tmp)&0x80) != 0;
	A = tmp & 0xFF;
	N_flag(A);
	Z_flag(A);
}

static void inst_AND()
{
	A &= *read_ptr();
	N_flag(A);
	Z_flag(A);
}

static void inst_ASL()
{
	uint8_t tmp = *read_ptr();
	SR.bits.carry = (tmp & 0x80) != 0;
	tmp <<= 1;
	N_flag(tmp);
	Z_flag(tmp);
	*write_ptr() = tmp;
}

static void inst_BCC()
{
	if (!SR.bits.carry) {
		take_branch();
	}
}

static void inst_BCS()
{
	if (SR.bits.carry) {
		take_branch();
	}
}

static void inst_BEQ()
{
	if (SR.bits.zero) {
		take_branch();
	}
}

static void inst_BIT()
{
	uint8_t tmp = *read_ptr();
	N_flag(tmp);
	Z_flag(tmp & A);
	SR.bits.overflow = (tmp & 0x40) != 0;
}

static void inst_BMI()
{
	if (SR.bits.sign) {
		take_branch();
	}
}

static void inst_BNE()
{
	if (!SR.bits.zero) {
		take_branch();
	}
}

static void inst_BPL()
{
	if (!SR.bits.sign) {
		take_branch();
	}
}

static void inst_BRK()
{
	uint16_t newPC;
	memcpy(&newPC, &memory[IRQ_VEC], sizeof(newPC));
	PC += 2;
	stack_push(PC >> 8);
	stack_push(PC & 0xFF);
	SR.bits.brk = 1;
	stack_push(SR.byte);
	SR.bits.interrupt = 1;
	PC = newPC;
	jumping = 1;
}

static void inst_BVC()
{
	if (!SR.bits.overflow) {
		take_branch();
	}
}

static void inst_BVS()
{
	if (SR.bits.overflow) {
		take_branch();
	}
}

static void inst_CLC()
{
	SR.bits.carry = 0;
}

static void inst_CLD()
{
	SR.bits.decimal = 0;
}

static void inst_CLI()
{
	SR.bits.interrupt = 0;
}

static void inst_CLV()
{
	SR.bits.overflow = 0;
}

static void inst_CMP()
{
	uint8_t operand = *read_ptr();
	uint8_t tmpDiff = A - operand;
	N_flag(tmpDiff);
	Z_flag(tmpDiff);
	SR.bits.carry = A >= operand;
}

static void inst_CPX()
{
	uint8_t operand = *read_ptr();
	uint8_t tmpDiff = X - operand;
	N_flag(tmpDiff);
	Z_flag(tmpDiff);
	SR.bits.carry = X >= operand;
}

static void inst_CPY()
{
	uint8_t operand = *read_ptr();
	uint8_t tmpDiff = Y - operand;
	N_flag(tmpDiff);
	Z_flag(tmpDiff);
	SR.bits.carry = Y >= operand;
}

static void inst_DEC()
{
	uint8_t tmp = *read_ptr();
	tmp--;
	N_flag(tmp);
	Z_flag(tmp);
	*write_ptr() = tmp;
}

static void inst_DEX()
{
	X--;
	N_flag(X);
	Z_flag(X);
}

static void inst_DEY()
{
	Y--;
	N_flag(Y);
	Z_flag(Y);
}

static void inst_EOR()
{
	A ^= *read_ptr();
	N_flag(A);
	Z_flag(A);
}

static void inst_INC()
{
	uint8_t tmp = *read_ptr();
	tmp++;
	N_flag(tmp);
	Z_flag(tmp);
	*write_ptr() = tmp;
}

static void inst_INX()
{
	X++;
	N_flag(X);
	Z_flag(X);
}

static void inst_INY()
{
	Y++;
	N_flag(Y);
	Z_flag(Y);
}

static void inst_JMP()
{
	PC = read_ptr() - memory;
	jumping = 1;
}

static void inst_JSR()
{
	uint16_t newPC = read_ptr() - memory;
	PC += 2;
	stack_push(PC >> 8);
	stack_push(PC & 0xFF);
	PC = newPC;
	jumping = 1;
}

static void inst_LDA()
{
	A = *read_ptr();
	N_flag(A);
	Z_flag(A);
}

static void inst_LDX()
{
	X = *read_ptr();
	N_flag(X);
	Z_flag(X);
}

static void inst_LDY()
{
	Y = *read_ptr();
	N_flag(Y);
	Z_flag(Y);
}

static void inst_LSR()
{
	uint8_t tmp = *read_ptr();
	SR.bits.carry = tmp & 1;
	tmp >>= 1;
	N_flag(tmp);
	Z_flag(tmp);
	*write_ptr() = tmp;
}

static void inst_NOP()
{
	// thrown away, just used to compute any extra cycles for the multi-byte
	// NOP statements
	read_ptr();
}

static void inst_ORA()
{
	A |= *read_ptr();
	N_flag(A);
	Z_flag(A);
}

static void inst_PHA()
{
	stack_push(A);
}

static void inst_PHP()
{
	union StatusReg pushed_sr;

	// PHP sets the BRK flag in the byte that is pushed onto the stack,
	// but doesn't affect the status register itself. this is slightly
	// unexpected, but it's what the real hardware does.
	//
	// See http://visual6502.org/wiki/index.php?title=6502_BRK_and_B_bit
	pushed_sr.byte = SR.byte;
	pushed_sr.bits.brk = 1;
	stack_push(pushed_sr.byte);
}

static void inst_PLA()
{
	A = stack_pull();
	N_flag(A);
	Z_flag(A);
}

static void inst_PLP()
{
	SR.byte = stack_pull();
	SR.bits.unused = 1;
	SR.bits.brk = 0;
}

static void inst_ROL()
{
	int tmp = (*read_ptr()) << 1;
	tmp |= SR.bits.carry & 1;
	SR.bits.carry = tmp > 0xFF;
	tmp &= 0xFF;
	N_flag(tmp);
	Z_flag(tmp);
	*write_ptr() = tmp;
}

static void inst_ROR()
{
	int tmp = *read_ptr();
	tmp |= SR.bits.carry << 8;
	SR.bits.carry = tmp & 1;
	tmp >>= 1;
	N_flag(tmp);
	Z_flag(tmp);
	*write_ptr() = tmp;
}

static void inst_RTI()
{
	SR.byte = stack_pull();
	SR.bits.unused = 1;
	PC = stack_pull();
	PC |= stack_pull() << 8;
	//PC += 1;
	jumping = 1;
}

static void inst_RTS()
{
	PC = stack_pull();
	PC |= stack_pull() << 8;
	PC += 1;
	jumping = 1;
}

static void inst_SBC()
{
	uint8_t operand = *read_ptr();
	unsigned int tmp, lo, hi;
	tmp = A - operand - 1 + (SR.bits.carry & 1);
	SR.bits.overflow = ((A^tmp)&(A^operand)&0x80) != 0;
	if (SR.bits.decimal) {
		lo = (A & 0x0f) - (operand & 0x0f) - 1 + SR.bits.carry;
		hi = (A >> 4) - (operand >> 4);
		if (lo & 0x10) lo -= 6, hi--;
		if (hi & 0x10) hi -= 6;
		A = (hi << 4) | (lo & 0x0f);
	}
	else {
		A = tmp & 0xFF;
	}
	SR.bits.carry = tmp < 0x100;
	N_flag(A);
	Z_flag(A);
}

static void inst_SEC()
{
	SR.bits.carry = 1;
}

static void inst_SED()
{
	SR.bits.decimal = 1;
}

static void inst_SEI()
{
	SR.bits.interrupt = 1;
}

static void inst_STA()
{
	*write_ptr() = A;
	extra_cycles = 0; // STA has no addressing modes that use the extra cycle
}

static void inst_STX()
{
	*write_ptr() = X;
}

static void inst_STY()
{
	*write_ptr() = Y;
}

static void inst_TAX()
{
	X = A;
	N_flag(X);
	Z_flag(X);
}

static void inst_TAY()
{
	Y = A;
	N_flag(Y);
	Z_flag(Y);
}

static void inst_TSX()
{
	X = SP;
	N_flag(X);
	Z_flag(X);
}

static void inst_TXA()
{
	A = X;
	N_flag(A);
	Z_flag(A);
}

static void inst_TXS()
{
	SP = X;
}

static void inst_TYA()
{
	A = Y;
	N_flag(A);
	Z_flag(A);
}

/* Addressing Implementations */

static uint8_t *get_IMPL(void)
{
	// dummy implementation; for completeness necessary for cycle counting NOP
	// instructions
	return &memory[0];
}

static uint8_t *get_IMM(void)
{
	return &memory[(uint16_t) (PC+1)];
}

static uint16_t get_uint16(void)
{ // used only as part of other modes
	uint16_t index;
	memcpy(&index, get_IMM(), sizeof(index)); // hooray for optimising compilers
	return index;
}

static uint8_t *get_ZP(void)
{
	return &memory[*get_IMM()];
}

static uint8_t *get_ZPX(void)
{
	return &memory[((*get_IMM()) + X) & 0xFF];
}

static uint8_t *get_ZPY(void)
{
	return &memory[((*get_IMM()) + Y) & 0xFF];
}

static uint8_t *get_ACC(void)
{
	return &A;
}

static uint8_t *get_ABS(void)
{
	return &memory[get_uint16()];
}

static uint8_t *get_ABSX(void)
{
	uint16_t ptr;
	ptr = (uint16_t)(get_uint16() + X);
	if ((uint8_t)ptr < X) extra_cycles ++;
	return &memory[ptr];
}

static uint8_t *get_ABSY(void)
{
	uint16_t ptr;
	ptr = (uint16_t)(get_uint16() + Y);
	if ((uint8_t)ptr < Y) extra_cycles ++;
	return &memory[ptr];
}

static uint8_t * get_IND(void)
{
	uint16_t ptr;
	memcpy(&ptr, get_ABS(), sizeof(ptr));
	return &memory[ptr];
}

static uint8_t * get_XIND(void)
{
	uint16_t ptr;
	ptr = ((* get_IMM()) + X) & 0xFF;
	if (ptr == 0xff) { // check for wraparound in zero page
		ptr = memory[ptr] + (memory[ptr & 0xff00] << 8);
	}
	else {
		memcpy(&ptr, &memory[ptr], sizeof(ptr));
	}
	return &memory[ptr];
}

static uint8_t * get_INDY(void)
{
	uint16_t ptr;
	ptr = * get_IMM();
	if (ptr == 0xff) { // check for wraparound in zero page
		ptr = memory[ptr] + (memory[ptr & 0xff00] << 8);
	}
	else {
		memcpy(&ptr, &memory[ptr], sizeof(ptr));
	}
	ptr += Y;
	if ((uint8_t)ptr < Y) extra_cycles ++;
	return &memory[ptr];
}

static uint8_t *get_REL(void)
{
	return &memory[(uint16_t) (PC + (int8_t) * get_IMM())];
}

static uint8_t *get_JMP_IND_BUG(void)
{
	uint8_t *addr;
	uint16_t ptr;

	ptr = get_uint16();
	if ((ptr & 0xff) == 0xff) {
		// Bug when crosses a page boundary. When using relative index ($xxff),
		// instead of using the last byte of the page and the first byte of the
		// next page, it uses the first byte of the same page. E.g. jmp ($baff)
		// would use the value at $baff as the LSB, but $ba00 as the high byte
		// instead of $bb00. This was fixed in the 65C02
		ptr = memory[ptr] + (memory[ptr & 0xff00] << 8);

	}
	else {
		addr = &memory[ptr];
		memcpy(&ptr, addr, sizeof(ptr));
	}
	return &memory[ptr];
}


void reset_cpu(int _a, int _x, int _y, int _sp, int _sr, int _pc)
{
	A = _a;
	X = _x;
	Y = _y;
	SP = _sp;
	
	SR.byte = _sr;
	SR.bits.interrupt = 1;
	SR.bits.unused = 1;
	
	if (_pc < 0)
		memcpy(&PC, &memory[-_pc], sizeof(PC));
	else
		PC = _pc;

	total_cycles = 0;
}

int load_rom(char *filename, int load_addr)
{
	int loaded_size, max_size;

	memset(memory, 0, sizeof(memory)); // clear ram first
	
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Error: could not open file\n");
		return -1;
	}
	
	max_size = 0x10000 - load_addr;
	loaded_size = (int)fread(&memory[load_addr], 1, (size_t)max_size, fp);
	fprintf(stderr, "Loaded $%04x bytes: $%04x - $%04x\n", loaded_size, load_addr, load_addr + loaded_size - 1);
	
	fclose(fp);
	return 0;
}

int step_cpu(int verbose) // returns cycle count
{
	inst = instructions[memory[PC]];

	if (verbose) {
		// almost match for NES dump for easier comparison
		printf("%04X  ", PC);
		if (lengths[inst.mode] == 3)
			printf("%02X %02X %02X", memory[PC], memory[PC+1], memory[PC+2]);
		else if (lengths[inst.mode] == 2)
			printf("%02X %02X   ", memory[PC], memory[PC+1]);
		else
			printf("%02X      ", memory[PC]);
		printf("  %-10s                      A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d\n", inst.mnemonic, A, X, Y, SR.byte, SP, (int)((total_cycles * 3) % 341));
	}

	jumping = 0;
	extra_cycles = 0;
	inst.function();
	if (jumping == 0) PC += lengths[inst.mode];

	// 7 cycle instructions (e.g. ROL $nnnn,X) don't have a penalty cycle for
	// crossing a page boundary.
	if (inst.cycles == 7) extra_cycles = 0;

	total_cycles += inst.cycles + extra_cycles;
	return inst.cycles + extra_cycles;
}

void save_memory(char *filename) { // dump memory for analysis (slows down emulation significantly)
	if (filename == NULL) filename = "memdump";
	FILE *fp = fopen(filename, "w");
	fwrite(&memory, sizeof(memory), 1, fp);
	fclose(fp);
}
