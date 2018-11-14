ll:

# Convert synth to final AT&T syntax includable single file for intro
#
# Steps:
#
# - Preprocess with nasm to process %includes and get a single file
# - Preprocessing for intel2gas
# - Extract %defines so that we can expand them ourselves for final version
# - Intel >- AT&T with intel2gas
# - Postprocessing to fix intel2gas errors

.PHONY: pp_defs.cache synth_pp.asm

synth_pp.asm: synth.asm
	nasm -e $^ >$@

pp_defs.cache:
	cat song.asm module*.asm synth.asm | ./intel2gas_pp.py defs $@

synth_att.asm: pp_defs.cache synth_pp.asm
	./intel2gas_pp.py pre <synth_pp.asm >synth_pp.asm.preprocessed
	intel2gas <synth_pp.asm.preprocessed | ./intel2gas_pp.py post pp_defs.cache >$@

synth_for_intro.asm: synth_pp.asm pp_defs.cache
	./intel2gas_pp.py pre <synth_pp.asm >synth_pp.asm.preprocessed
	intel2gas <synth_pp.asm.preprocessed | ./intel2gas_pp.py --full post pp_defs.cache >$@

synth_att.o: synth_att.asm
	as -32 -o $@ $^

synth.o: synth.asm song.asm module*.asm
	nasm -f elf32 -o $@ $<

# Test that Intel -> AT&T conversion was truly lossless; compilation from
# synth.asm and from processed includable AT&T deliverable must yield
# identical text segments
.PHONY: test
test: synth_att.o synth.o
	objdump -M intel -d synth.o | tail -n +3 >synth.o.text
	objdump -M intel -d synth_att.o | tail -n +3 >synth_att.o.text
	diff synth.o.text synth_att.o.text

clean:
	rm -f *.o *.text pp_defs.cache synth*.asm rendertest render_params.h

.PHONY: build_rendertest
_build_rendertest: rendertest.cpp synth_att.o
	g++ -m32 -o rendertest $^ -lsndfile

rendertest: sizes rendertest.cpp synth_att.o
	docker run -v $(CURDIR):/src buster-32-buildenv:latest \
		make -C /src _build_rendertest
	time ./rendertest out.wav

# Targets for showing estimates of final synth size
synth.bin.text: synth_att.o
	objcopy --dump-section .text=$@ $^

synth.bin.data: synth_att.o
	objcopy --dump-section .data=$@ $^

synth.packed: synth.bin.text synth.bin.data
	cat $^ | gzip -c - >$@

.PHONY sizes:
sizes: synth.bin.text synth.bin.data synth.packed
	-echo
	-echo "-- segment sizes --"
	ls -l $^
	-echo
