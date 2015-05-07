include ../Makefile.in

OBJS = Schema.o Parser.o SchemaManager.o

all: $(OBJS)

Schema.o: Schema.cpp Schema.hpp Makefile
	$(CXX) $(CXXFLAGS) -c Schema.cpp -o Schema.o

Parser.o: Parser.cpp Parser.hpp Makefile
	$(CXX) $(CXXFLAGS) -c Parser.cpp -o Parser.o

%.o: %.cpp %.hpp
	$(CXX) -c $(CFLAGS) -o $@ $<

parser: Schema.o Parser.o Schema.hpp Types.hpp parser.cpp Makefile
	$(CXX) $(CXXFLAGS) Parser.o Schema.o parser.cpp -o parser

clean:
	$(RM) -rf $(OBJS) parser
