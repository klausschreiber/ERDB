include ../Makefile.in

OBJS = buffertest.o schematest.o slottedpagestest.o slottedtest.o operatortest.o hashjointest.o

all: $(OBJS)

%.o: %.cpp
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	$(RM) -rf $(OBJS)
