#define CTRL_ADDR 0xA000
#define DATA_ADDR 0xA001

struct UartStatusBits{
	int RDRF:1; // bit 0
	int TDRE:1;
	int DCD:1;
	int CTS:1;
	int FE:1;
	int OVRN:1;
	int PE:1;
	int IRQ:1; // bit 7
};

union UartStatusReg {
	struct UartStatusBits bits;
	uint8_t byte;
};

union UartStatusReg uart_SR;

uint8_t incoming_char;

void init_uart();

void step_uart();
