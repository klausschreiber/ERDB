include ../Makefile.in

OBJS = BTree.o

all: $(OBJS)

BTree.o: BTree.cpp BTree.hpp Makefile
	$(CXX) $(CXXFLAGS) -c BTree.cpp -o BTree.o

%.o: %.cpp %.hpp
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	$(RM) -rf $(OBJS)
