include ../Makefile.in

OBJS = Schema.o SchemaManager.o

all: $(OBJS)

Schema.o: Schema.cpp Schema.hpp Makefile
	$(CXX) $(CXXFLAGS) -c Schema.cpp -o Schema.o

Parser.o: Parser.cpp Parser.hpp Makefile
	$(CXX) $(CXXFLAGS) -c Parser.cpp -o Parser.o

%.o: %.cpp %.hpp
	$(CXX) -c $(CFLAGS) -o $@ $<

parser: Schema.o Parser.o Schema.hpp Types.hpp parser-test.cpp Makefile
	$(CXX) $(CXXFLAGS) Parser.o Schema.o parser-test.cpp -o parser-test

clean:
	$(RM) -rf $(OBJS) parser-test
