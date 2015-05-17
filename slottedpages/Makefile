include ../Makefile.in

OBJS = SPSegment.o Record.o

all: $(OBJS)

SPSegment.o: SPSegment.cpp SPSegment.hpp Makefile
	$(CXX) $(CXXFLAGS) -c SPSegment.cpp -o SPSegment.o

Record.o: Record.hpp Record.cpp Makefile
	$(CXX) $(CXXFLAGS) -c Record.cpp -o Record.o

%.o: %.cpp %.hpp
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	$(RM) -rf $(OBJS)
