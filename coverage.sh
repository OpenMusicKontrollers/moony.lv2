#!/bin/sh

rm -f *.profraw *.profdata

LLVM_PROFILE_FILE=moony_test.profraw ./moony_test ../test/moony_test.lua
LLVM_PROFILE_FILE=moony_overflow.profraw ./moony_test ../test/moony_overflow.lua 0
LLVM_PROFILE_FILE=moony_manual.profraw ./moony_test ../test/moony_manual.lua
LLVM_PROFILE_FILE=moony_presets.profraw ./moony_test ../test/moony_presets.lua

llvm-profdata merge -sparse *.profraw -o moony.profdata

for file in ../api/api_*.c;
do
	llvm-cov $1 ./moony_test -instr-profile=moony.profdata $file;
done

llvm-cov $1 ./moony_test -instr-profile=moony.profdata
