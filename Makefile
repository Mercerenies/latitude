
ifndef BOOST
  BOOST='D:/boost/boost_1_60_0'
  #BOOST="/root/Downloads/boost_1_60_0"
endif

CCFLAGS=-c -Wall
CXXFLAGS=-I $(BOOST) -c -Wall -std=c++1y
LINKFLAGS=-I $(BOOST) -Wall -std=c++1y
LINK=$(CXX) $(LINKFLAGS) -Wall -std=c++1y
FILES=Proto.o Standard.o Scanner.o Parser.o main.o Reader.o Stream.o Garnish.o GC.o Symbol.o REPL.o Number.o Process.o Bytecode.o

all: Project

debug:	CXXFLAGS += -g
debug:	CCFLAGS += -g
debug:	LINKFLAGS += -g
debug:	Project

profile:	CXXFLAGS += -pg
profile:	CCFLAGS += -pg
profile:	LINKFLAGS += -pg
profile:	Project

Project:	$(FILES)
	$(LINK) $(FILES)

clean:
	rm *.o
	rm lex.yy.c lex.yy.h Parser.tab.c Parser.tab.h

Proto.o:	Proto.cpp Proto.hpp Stream.hpp GC.hpp Symbol.hpp Standard.hpp Number.hpp Reader.hpp Garnish.hpp Macro.hpp Parser.tab.c Process.hpp Bytecode.hpp
	$(CXX) $(CXXFLAGS) Proto.cpp

Standard.o:	Standard.cpp Standard.hpp Proto.hpp Process.hpp Reader.hpp Stream.hpp Garnish.hpp Macro.hpp Parser.tab.c GC.hpp Bytecode.hpp
	$(CXX) $(CXXFLAGS) Standard.cpp

Scanner.o:	lex.yy.c lex.yy.h
	$(CC) $(CCFLAGS) lex.yy.c -o Scanner.o

Parser.o:	Parser.tab.c
	$(CXX) $(CXXFLAGS) Parser.tab.c -o Parser.o

lex.yy.c:	Scanner.flex Parser.tab.c
	flex Scanner.flex

Parser.tab.c:	Parser.y
	bison -d Parser.y

Reader.o:	Reader.cpp Reader.hpp Parser.tab.c Symbol.hpp Standard.hpp Garnish.hpp Macro.hpp Proto.hpp Process.hpp Bytecode.hpp
	$(CXX) $(CXXFLAGS) Reader.cpp

Stream.o:	Stream.cpp Stream.hpp
	$(CXX) $(CXXFLAGS) Stream.cpp

Garnish.o:	Garnish.cpp Garnish.hpp Proto.hpp Stream.hpp Reader.hpp Macro.hpp Process.hpp Bytecode.hpp
	$(CXX) $(CXXFLAGS) Garnish.cpp

GC.o:	GC.cpp GC.hpp Proto.hpp Process.hpp
	$(CXX) $(CXXFLAGS) GC.cpp

Symbol.o:	Symbol.cpp Symbol.hpp
	$(CXX) $(CXXFLAGS) Symbol.cpp

Number.o:	Number.cpp Number.hpp
	$(CXX) $(CXXFLAGS) Number.cpp

REPL.o:	REPL.cpp REPL.hpp Proto.hpp Reader.hpp Symbol.hpp Garnish.hpp Standard.hpp GC.hpp Process.hpp Stream.hpp Bytecode.hpp
	$(CXX) $(CXXFLAGS) REPL.cpp

Process.o:	Process.cpp Process.hpp Stream.hpp
	$(CXX) $(CXXFLAGS) Process.cpp

Bytecode.o:	Bytecode.cpp Bytecode.hpp Symbol.hpp Number.hpp Proto.hpp Reader.hpp Garnish.hpp
	$(CXX) $(CXXFLAGS) Bytecode.cpp

main.o:	main.cpp lex.yy.h Standard.hpp Reader.hpp Garnish.hpp GC.hpp REPL.hpp Bytecode.hpp
	$(CXX) $(CXXFLAGS) main.cpp
