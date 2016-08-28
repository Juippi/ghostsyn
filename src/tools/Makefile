include ../common.mk

OBJS=synthtool.o ../synth/synth.o
LIBS=../synthdata/sythdata.a
CXXFLAGS=$(COMMON_CXXFLAGS) -I../synthdata/ -I../synth/
LDFLAGS=$(COMMON_LDFLAGS) -ljsoncpp -lsynthdata -lsndfile

all: synthtool

synthtool: $(OBJS)
	LIBRARY_PATH=../synthdata/ g++ -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f synthtool $(OBJS) *.d
