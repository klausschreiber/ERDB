gen: gen.cpp Makefile
	g++ -g -O2 -Werror gen.cpp -o gen

sort: sort.cpp Makefile
	g++ -g -O2 -Wall sort.cpp -o sort

clean:
	rm -rf gen
