
ifndef BOOST
  #BOOST='D:/boost/boost_1_60_0'
  BOOST=$(shell readlink -f ~/D/Downloads/boost_1_64_0)
endif

CCFLAGS=-c -Wall
CXXFLAGS=-I $(BOOST) -c -Wall -std=gnu++1y
LINKFLAGS=-I $(BOOST) -Wall -std=gnu++1y
LINK=$(CXX) $(LINKFLAGS) -Wall -std=gnu++1y -o latitude
FILES=Proto.o Standard.o Scanner.o Parser.o main.o Reader.o Stream.o Garnish.o GC.o Symbol.o REPL.o Number.o Process.o Bytecode.o Header.o Instructions.o Environment.o Pathname.o Allocator.o Unicode.o Args.o Assembler.o

all: Project

debug:	CXXFLAGS += -g
debug:	CCFLAGS += -g
debug:	LINKFLAGS += -g
debug:	Project

profile:	CXXFLAGS += -pg
profile:	CCFLAGS += -pg
profile:	LINKFLAGS += -pg
profile:	Project

release:	CXXFLAGS += -O3
release:	CCFLAGS += -O3
release:	LINKFLAGS += -O3
release:	Project

Project:	$(FILES)
	$(LINK) $(FILES)

clean:
	rm *.o
	rm lex.yy.c lex.yy.h Parser.tab.c Parser.tab.h

Proto.o:	Proto.cpp Proto.hpp Stream.hpp GC.hpp Symbol.hpp Standard.hpp Number.hpp Reader.hpp Garnish.hpp Macro.hpp Parser.tab.c Process.hpp Bytecode.hpp Instructions.hpp Stack.hpp
	$(CXX) $(CXXFLAGS) Proto.cpp

Standard.o:	Standard.cpp Standard.hpp Proto.hpp Process.hpp Reader.hpp Stream.hpp Garnish.hpp Macro.hpp Parser.tab.c GC.hpp Bytecode.hpp Instructions.hpp Assembler.hpp Environment.hpp Pathname.hpp Stack.hpp
	$(CXX) $(CXXFLAGS) Standard.cpp

Scanner.o:	lex.yy.c lex.yy.h
	$(CC) $(CCFLAGS) lex.yy.c -o Scanner.o

Parser.o:	Parser.tab.c
	$(CXX) $(CXXFLAGS) Parser.tab.c -o Parser.o

lex.yy.c:	Scanner.flex Parser.tab.c
	flex Scanner.flex

Parser.tab.c:	Parser.y
	bison -d Parser.y

Reader.o:	Reader.cpp Reader.hpp Parser.tab.c Symbol.hpp Standard.hpp Garnish.hpp Macro.hpp Proto.hpp Process.hpp Bytecode.hpp Instructions.hpp Assembler.hpp Stack.hpp
	$(CXX) $(CXXFLAGS) Reader.cpp

Stream.o:	Stream.cpp Stream.hpp
	$(CXX) $(CXXFLAGS) Stream.cpp

Garnish.o:	Garnish.cpp Garnish.hpp Proto.hpp Stream.hpp Reader.hpp Macro.hpp Process.hpp Bytecode.hpp Instructions.hpp Assembler.hpp Stack.hpp
	$(CXX) $(CXXFLAGS) Garnish.cpp

GC.o:	GC.cpp GC.hpp Proto.hpp Process.hpp Bytecode.hpp Instructions.hpp Allocator.hpp Stack.hpp
	$(CXX) $(CXXFLAGS) GC.cpp

Symbol.o:	Symbol.cpp Symbol.hpp
	$(CXX) $(CXXFLAGS) Symbol.cpp

Number.o:	Number.cpp Number.hpp
	$(CXX) $(CXXFLAGS) Number.cpp

REPL.o:	REPL.cpp REPL.hpp Proto.hpp Reader.hpp Symbol.hpp Garnish.hpp Standard.hpp GC.hpp Process.hpp Stream.hpp Bytecode.hpp Instructions.hpp Pathname.hpp Stack.hpp
	$(CXX) $(CXXFLAGS) REPL.cpp

Process.o:	Process.cpp Process.hpp Stream.hpp Platform.hpp
	$(CXX) $(CXXFLAGS) Process.cpp

Bytecode.o:	Bytecode.cpp Bytecode.hpp Symbol.hpp Number.hpp Proto.hpp Reader.hpp Garnish.hpp Header.hpp Instructions.hpp Instructions.hpp Assembler.hpp Stack.hpp
	$(CXX) $(CXXFLAGS) Bytecode.cpp

Header.o:	Header.cpp Header.hpp
	$(CXX) $(CXXFLAGS) Header.cpp

Instructions.o:	Instructions.cpp Instructions.hpp
	$(CXX) $(CXXFLAGS) Instructions.cpp

Environment.o:	Environment.cpp Environment.hpp Platform.hpp
	$(CXX) $(CXXFLAGS) Environment.cpp

Pathname.o: Pathname.cpp Pathname.hpp Platform.hpp
	$(CXX) $(CXXFLAGS) Pathname.cpp

Allocator.o:	Allocator.cpp Allocator.hpp Proto.hpp Process.hpp Bytecode.hpp Instructions.hpp Stack.hpp
	$(CXX) $(CXXFLAGS) Allocator.cpp

Unicode.o:	Unicode.cpp Unicode.hpp
	$(CXX) $(CXXFLAGS) Unicode.cpp

Args.o:	Args.cpp Args.hpp
	$(CXX) $(CXXFLAGS) Args.cpp

Assembler.o:	Assembler.cpp Assembler.hpp Instructions.hpp
	$(CXX) $(CXXFLAGS) Assembler.cpp

main.o:	main.cpp lex.yy.h Standard.hpp Reader.hpp Garnish.hpp GC.hpp REPL.hpp Bytecode.hpp Instructions.hpp Proto.hpp Stack.hpp Args.hpp Pathname.hpp
	$(CXX) $(CXXFLAGS) main.cpp
