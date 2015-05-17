include ../Makefile.in

OBJS = BufferManager.o BufferFrame.o

all: $(OBJS)

%.o: %.cpp %.hpp
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	$(RM) -rf $(OBJS)
