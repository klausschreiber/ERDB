include ../Makefile.in

OBJS = Register.o TableScan.o

all: $(OBJS)


%.o: %.cpp %.hpp
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	$(RM) -rf $(OBJS)
