CFLAGS = -Wall -Wpedantic -g -std=gnu99
LDFLAGS = -Ofast

OBJ := 6502-emu.o 6502.o 6850.o

all: 6502-emu

debug: CFLAGS += -DDEBUG
debug: 6502-emu

6502-emu: $(OBJ)

clean:
	$(RM) 6502-emu $(OBJ)

test: 6502-emu
	./6502-emu examples/ehbasic.rom
