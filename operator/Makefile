include ../Makefile.in

OBJS = Register.o TableScan.o Print.o Projection.o Selection.o HashJoin.o

all: $(OBJS)


%.o: %.cpp %.hpp
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	$(RM) -rf $(OBJS)
