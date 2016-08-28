include ../common.mk

OBJS=pattern.o tracker_data.o module.o bin_section.o
CXXFLAGS=$(COMMON_CXXFLAGS) -I/usr/include/jsoncpp

libsynthdata.a: $(OBJS)
	ar cr $@ $^

clean:
	rm -f libsynthdata.a $(OBJS) *.d
