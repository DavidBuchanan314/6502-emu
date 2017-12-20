#!/usr/bin/env python

# NES test rom: https://wiki.nesdev.com/w/index.php/Emulator_tests

# other tests:
#  https://github.com/Klaus2m5/6502_65C02_functional_tests
#  https://github.com/christopherpow/nes-test-roms

# other simulators:
#  http://piumarta.com/software/lib6502/
#  https://github.com/dennis-chen/6502-Emu/blob/master/src/cpu.c

# references
#  https://wiki.nesdev.com/w/index.php/Status_flags
#  http://www.emulator101.com/6502-addressing-modes.html
#  http://nesdev.com/6502.txt

# now passes the NES torture test, docs at http://www.qmtpro.com/~nes/misc/nestest.txt
# NES ROM available at http://nickmass.com/images/nestest.nes
# log for passing test available at http://www.qmtpro.com/~nes/misc/nestest.log


# Format of test log from the NES test
# C761  4C 68 C7  JMP $C768                       A:40 X:00 Y:00 P:24 SP:FB CYC:195


with open('test.log') as f1, open('test/nestest-real-6502.log') as f2:
    for i, (line1, line2) in enumerate(zip(f1, f2)):
        fail = False
        if line1[0:19] != line2[0:19]:
            fail = True
        if line1[48:81] != line2[48:81]:
            fail = True

        if fail and "NOP" not in line2:
            print("first diff: line #%d: %s\ngen: %s\nnes: %s\n" % (i, line2[0:19], line1, line2))
            print("   gen: group 1: %s group 2: %s" % (line1[0:19], line1[48:81]))
            print("   nes: group 1: %s group 2: %s" % (line2[0:19], line2[48:81]))
            break

