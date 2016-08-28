all: synth.o

synth.o: synth.asm
	nasm -f elf32 -DTRACKER_EMBED=1 -o $@ $^

clean:
	rm -f *.o
