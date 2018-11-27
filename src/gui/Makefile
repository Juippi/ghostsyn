include ../common.mk

LIBS=../synthdata/sythdata.a
CXXFLAGS=$(COMMON_CXXFLAGS) -D_REENTRANT -I/usr/include/SDL2 -I../synthdata/ -I../synth/
LDFLAGS=$(COMMON_LDFLAGS) -ljsoncpp -lSDL2_ttf -lSDL2 -lGL -ljsoncpp -lsynthdata

OBJS=editor_window.o main_window.o main.o fonts.o patch_editor_window.o tracker_window.o audio.o utils.o \
	../synth/synth.o

editor: $(OBJS)
	LIBRARY_PATH=../synthdata g++ -o editor $(OBJS) $(LDFLAGS)

clean:
	rm -f editor *.o *.d
