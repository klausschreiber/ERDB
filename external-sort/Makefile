include ../Makefile.in

all: gen sort

gen: gen.cpp
	$(CXX) $(CFLAGS) gen.cpp -o gen

sort: sort.cpp
	$(CXX) $(CFLAGS) sort.cpp -o sort

clean:
	$(RM) -rf gen sort
