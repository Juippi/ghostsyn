INTRODIR := $(CURDIR)/../region_de_magallanes
FREEBSD_VM := fb32
INTRODIR_VM := ~/git/region_de_magallanes/src/
COMPILE_SONG ?= foo.json

all: build

build:
	docker run -v $(PWD)/src:/src \
		buster-32-buildenv:latest $(MAKE) -j4 -C src/ synth/synth.o editor tools

buildshell:
	docker run -ti -v $(PWD)/src:/src \
		buster-32-buildenv:latest /bin/bash

clean:
	make -C src/ clean
	make -C compile/ clean
	make -C doc/ clean

.PHONY: compile
compile: build
	src/tools/synthtool -c $(COMPILE_SONG) compile/song.asm compile/song_params.h
	cp -v src/synth/*.asm compile/
	make -C compile test
	make -C compile rendertest
	make -C compile sizes

install: compile
	make -C compile synth_for_intro.asm
	cp -v compile/synth_for_intro.asm $(INTRODIR)/src/synth.asm
	cp -v compile/song_params.h $(INTRODIR)/src/synth.h
	# cp -v compile/out.wav $(INTRODIR)/region_de_magallanes.wav

introtest: install
	scp -v compile/synth_for_intro.asm $(FREEBSD_VM):$(INTRODIR_VM)/synth.asm
	scp -v compile/song_params.h $(FREEBSD_VM):$(INTRODIR_VM)/synth.h
	ssh $(FREEBSD_VM) "cd ~/git/adarkar_wastes && /bin/sh ./compile.sh"

docs:
	make -C doc/
