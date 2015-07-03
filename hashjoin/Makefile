include ../Makefile.in

OBJS = hashjoin.o

all: $(OBJS)


%.o: %.cpp
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	$(RM) -rf $(OBJS)
