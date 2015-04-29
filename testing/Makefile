include ../Makefile.in

OBJS = buffertest.o

all: $(OBJS)

%.o: %.cpp
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	$(RM) -rf $(OBJS)
