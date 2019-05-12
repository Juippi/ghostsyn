all: synth.o testprog

synth.o: synth.asm module_oscillator.asm module_envelope.asm module_filter_v2.asm module_compressor.asm module_reverb.asm
	nasm -w+error=orphan-labels -f elf32 -DTRACKER_EMBED=1 -o $@ $<

testprog: synth.o testprog.cpp
	g++ -m32 -o $@ $^ -lsndfile

clean:
	rm -f *.o
