gen: gen.cpp Makefile
	g++ -g -O2 -Werror gen.cpp -o gen

sort: sort.cpp Makefile
	g++ -g -O2 -Wall -std=c++11 sort.cpp -o sort

clean:
	rm -rf gen
