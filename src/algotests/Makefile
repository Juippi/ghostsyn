include ../common.mk

OBJS=algotests.o
CXXFLAGS=$(COMMON_CXXFLAGS) -Wno-unused
LDFLAGS=$(COMMON_LDFLAGS) -lsndfile

all: algotests

algotests: $(OBJS)
	g++ -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f algotests $(OBJS) *.d
