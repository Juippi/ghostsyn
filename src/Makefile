all: synth/synth

.PHONY: editor
editor: synthdata/synthdata.a synth/synth.o
	$(MAKE) -C gui/ editor

.PHONY: tools
tools: synthdata/synthdata.a synth/synth.o
	$(MAKE) -C tools/

.PHONY: algotests
algotests:
	$(MAKE) -C algotests/

.PHONY: synth/synth.o
synth/synth.o:
	$(MAKE) -C synth/

.PHONY: synthdata/synthdata.a
synthdata/synthdata.a:
	$(MAKE) -C synthdata/ libsynthdata.a

.PHONY: clean
clean:
	$(MAKE) -C gui/ clean
	$(MAKE) -C tools/ clean
	$(MAKE) -C synth/ clean
	$(MAKE) -C synthdata clean
	$(MAKE) -C algotests/ clean
