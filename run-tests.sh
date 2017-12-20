echo "Running NES test"
echo "***** Note: successful NES test will fail at the first illegal instruction, LAX at line 5259"
./6502-emu -v -s 0xfd -r 0xc000 -c 300000 test/nestest-real-6502.rom > test.log; python compare.py
echo
echo "Running decimal mode test"
./6502-emu -s 0xfd -c 3000000 -l 0x000a -r 0x1000 test/6502_functional_test+decimal.bin
