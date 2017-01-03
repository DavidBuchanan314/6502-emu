# 6502-emu
A relatively bug-free and simple 6502 emulator. It is capable of running ehBasic.

Many 6502 emulators make extensive use of macros, which I personally think makes
code harder to understand. At the other extreme, some emulators implement each
addressing mode of each instruction separately, which is a big waste of time and
makes the code harder to maintain.

I chose a middle-ground, and used lookup tables for instruction decoding and address
decoding. This also gets rid of the common "giant opcode switch statement" (which
can be seen in my CHIP8 emulator project).

### Usage Example:

```
[david@D-ARCH 6502-emu]$ ./6502-emu examples/ehbasic.rom 

Enhanced 6502 BASIC 2.22 (c) Lee Davison
[C]old/[W]arm ?

Memory size ? 

40191 Bytes free

Enhanced BASIC 2.22

Ready
10 X1=59
15 Y1=21
20 I1=-1.0
23 I2=1.0
26 R1=-2.0
28 R2=1.0
30 S1=(R2-R1)/X1
35 S2=(I2-I1)/Y1
40 FOR Y=0 TO Y1
50 I3=I1+S2*Y
60 FOR X=0 TO X1
70 R3=R1+S1*X
73 Z1=R3
76 Z2=I3
80 FOR N=0 TO 30
90 A=Z1*Z1
95 B=Z2*Z2
100 IF A+B>4.0 GOTO 130
110 Z2=2*Z1*Z2+I3
115 Z1=A-B+R3
120 NEXT N
130 PRINT CHR$(63-N);
140 NEXT X
150 PRINT
160 NEXT Y
170 END

RUN
??????>>>>>===============<<<<<<;;;:7143;<<<<====>>>>>>>>>>>
?????>>>================<<<<<<<;;;984+18:;;<<<<=====>>>>>>>>
????>>>===============<<<<<<<;;::85    )/:;;;;<<=====>>>>>>>
???>>===============<<<<<<;:9999875     689::::;<<=====>>>>>
??>>=============<<<<;;;;::7/ '3           56446;<======>>>>
??>===========<<<;;;;;;:::863                 +8:;<======>>>
?>========<<<;6::::::::997!                   &89;<<======>>
?====<<<<<;;;:83567.678874                      ,:<<=======>
?=<<<<<<;;;;:986'      /4                       +:<<<======>
?<<<<<<;;::8675(        (                       9;<<<======>
?;;:999:88460                                 '9:;<<<======>
?;;:999:88460                                 '9:;<<<======>
?<<<<<<;;::8675(        (                       9;<<<======>
?=<<<<<<;;;;:986'      /4                       +:<<<======>
?====<<<<<;;;:83567.678874                      ,:<<=======>
?>========<<<;6::::::::997!                   &89;<<======>>
??>===========<<<;;;;;;:::863                 +8:;<======>>>
??>>=============<<<<;;;;::7/ '3           56446;<======>>>>
???>>===============<<<<<<;:9999875     689::::;<<=====>>>>>
????>>>===============<<<<<<<;;::85    )/:;;;;<<=====>>>>>>>
?????>>>================<<<<<<<;;;984+18:;;<<<<=====>>>>>>>>
??????>>>>>===============<<<<<<;;;:7143;<<<<====>>>>>>>>>>>

Ready

```

### TODO:

- Decimal mode.
- Cycle-accurate timing.
- GDB interface for easy debugging of 6502 programs (and the emulator itself).
- Implement more hardware devices (mainly a 6522, also some character/graphics LCDs).
- Turn it into a NES emulator.
