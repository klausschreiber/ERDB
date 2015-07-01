include ../Makefile.in

OBJS = hashjoin.o hashjointest.o

all: $(OBJS)


%.o: %.cpp
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	$(RM) -rf $(OBJS)
