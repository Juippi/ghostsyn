INTRODIR := $(CURDIR)/../cassini
FREEBSD_VM := fb32
INTRODIR_VM := ~/git/adarkar_wastes/src/

all: build

build:
	docker run -v $(PWD)/src:/src \
		buster-32-buildenv:latest $(MAKE) -j4 -C src/ editor tools algotests

buildshell:
	docker run -ti -v $(PWD)/src:/src \
		buster-32-buildenv:latest /bin/bash

clean:
	make -C src/ clean
	make -C compile/ clean
	make -C doc/ clean

.PHONY: compile
compile: build
	src/tools/synthtool -c src/gui/testsong.json compile/song.asm compile/song_params.h
	cp -v src/synth/*.asm compile/
	make -C compile test
	make -C compile rendertest
	make -C compile sizes

install: compile
	make -C compile synth_for_intro.asm
	cp -v compile/synth_for_intro.asm $(INTRODIR)/src/synth.asm
	cp -v compile/song_params.h $(INTRODIR)/src/synth.h
	cp -v compile/out.wav $(INTRODIR)/cassini.wav

introtest: install
	scp -v compile/synth_for_intro.asm $(FREEBSD_VM):$(INTRODIR_VM)/synth.asm
	scp -v compile/song_params.h $(FREEBSD_VM):$(INTRODIR_VM)/synth.h
	ssh $(FREEBSD_VM) "cd ~/git/adarkar_wastes && /bin/sh ./compile.sh"

docs:
	make -C doc/
