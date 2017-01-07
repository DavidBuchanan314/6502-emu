#define CTRL_ADDR 0xA000
#define DATA_ADDR 0xA001

struct UartStatusBits{
	unsigned char RDRF:1; // bit 0
	unsigned char TDRE:1;
	unsigned char DCD:1;
	unsigned char CTS:1;
	unsigned char FE:1;
	unsigned char OVRN:1;
	unsigned char PE:1;
	unsigned char IRQ:1; // bit 7
};

union UartStatusReg {
	struct UartStatusBits bits;
	uint8_t byte;
};

union UartStatusReg uart_SR;

uint8_t incoming_char;

void init_uart();

void step_uart();
