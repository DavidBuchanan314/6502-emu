CC=gcc
CFLAGS=-Wall -Ofast -std=gnu99

all: 6502-emu

6502-emu: main.o 6502.o 6850.o
	$(CC) -o 6502-emu main.c 6502.o 6850.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

6502.o: 6502.c
	$(CC) $(CFLAGS) -c 6502.c

6850.o: 6850.c
	$(CC) $(CFLAGS) -c 6850.c

clean:
	rm *.o 6502-emu

test: 6502-emu
	./6502-emu examples/ehbasic.rom
