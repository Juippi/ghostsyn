all: synth.o

synth.o: synth.asm module_oscillator.asm module_envelope.asm module_filter.asm module_compressor.asm module_delay.asm module_chorus.asm module_reverb.asm
	nasm -w+error=orphan-labels -f elf32 -DTRACKER_EMBED=1 -o $@ $<

clean:
	rm -f *.o
