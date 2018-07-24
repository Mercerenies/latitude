
FILES=Proto.o Standard.o Scanner.o Parser.o main.o Reader.o Stream.o Garnish.o GC.o Symbol.o REPL.o Number.o Process.o Bytecode.o Header.o Instructions.o Environment.o Pathname.o Allocator.o Unicode.o Args.o Assembler.o pl_Unidata.o Operator.o Optimizer.o CUnicode.o Protection.o Dump.o Parents.o Precedence.o Input.o Base.o Statics.o

all:	Project

debug:	Project

profile:	Project

release:	Project

Project:	$(FILES)
	$(LINK) -o ../latitude $(FILES)

Proto.o:	Proto.cpp Proto.hpp Protection.hpp Stream.hpp GC.hpp Symbol.hpp Standard.hpp Number.hpp Reader.hpp Garnish.hpp Macro.hpp Parser.tab.c Process.hpp Bytecode.hpp Instructions.hpp Stack.hpp Allocator.hpp
	$(CXX) $(CXXFLAGS) Proto.cpp

Standard.o:	Standard.cpp Standard.hpp Proto.hpp Protection.hpp Process.hpp Reader.hpp Stream.hpp Garnish.hpp Macro.hpp Parser.tab.c GC.hpp Bytecode.hpp Instructions.hpp Assembler.hpp Environment.hpp Pathname.hpp Stack.hpp Platform.hpp Unicode.hpp pl_Unidata.h Base.hpp
	$(CXX) $(CXXFLAGS) Standard.cpp

Scanner.o:	lex.yy.c lex.yy.h
	$(CC) $(CCFLAGS) lex.yy.c -o Scanner.o

Parser.o:	Parser.tab.c
	$(CXX) $(CXXFLAGS) Parser.tab.c -o Parser.o

lex.yy.c:	Scanner.flex Parser.tab.c Operator.h CUnicode.h
	flex Scanner.flex

Parser.tab.c:	Parser.y
	bison -d Parser.y

Reader.o:	Reader.cpp Reader.hpp Parser.tab.c Symbol.hpp Standard.hpp Garnish.hpp Macro.hpp Proto.hpp Protection.hpp Process.hpp Bytecode.hpp Instructions.hpp Assembler.hpp Stack.hpp Optimizer.hpp Pathname.hpp Base.hpp Serialize.hpp
	$(CXX) $(CXXFLAGS) Reader.cpp

Stream.o:	Stream.cpp Stream.hpp
	$(CXX) $(CXXFLAGS) Stream.cpp

Garnish.o:	Garnish.cpp Garnish.hpp Proto.hpp Protection.hpp Stream.hpp Reader.hpp Macro.hpp Process.hpp Bytecode.hpp Instructions.hpp Assembler.hpp Stack.hpp Base.hpp
	$(CXX) $(CXXFLAGS) Garnish.cpp

GC.o:	GC.cpp GC.hpp Proto.hpp Protection.hpp Process.hpp Bytecode.hpp Instructions.hpp Allocator.hpp Stack.hpp
	$(CXX) $(CXXFLAGS) GC.cpp

Symbol.o:	Symbol.cpp Symbol.hpp
	$(CXX) $(CXXFLAGS) Symbol.cpp

Number.o:	Number.cpp Number.hpp
	$(CXX) $(CXXFLAGS) Number.cpp

REPL.o:	REPL.cpp REPL.hpp Proto.hpp Protection.hpp Reader.hpp Symbol.hpp Garnish.hpp Standard.hpp GC.hpp Process.hpp Stream.hpp Bytecode.hpp Instructions.hpp Pathname.hpp Stack.hpp
	$(CXX) $(CXXFLAGS) REPL.cpp

Process.o:	Process.cpp Process.hpp Stream.hpp Platform.hpp
	$(CXX) $(CXXFLAGS) Process.cpp

Bytecode.o:	Bytecode.cpp Bytecode.hpp Symbol.hpp Number.hpp Proto.hpp Protection.hpp Reader.hpp Garnish.hpp Header.hpp Instructions.hpp Instructions.hpp Assembler.hpp Stack.hpp GC.hpp Base.hpp Serialize.hpp
	$(CXX) $(CXXFLAGS) Bytecode.cpp

Header.o:	Header.cpp Header.hpp Serialize.hpp
	$(CXX) $(CXXFLAGS) Header.cpp

Instructions.o:	Instructions.cpp Instructions.hpp Serialize.hpp
	$(CXX) $(CXXFLAGS) Instructions.cpp

Environment.o:	Environment.cpp Environment.hpp Platform.hpp
	$(CXX) $(CXXFLAGS) Environment.cpp

Pathname.o: Pathname.cpp Pathname.hpp Platform.hpp
	$(CXX) $(CXXFLAGS) Pathname.cpp

Allocator.o:	Allocator.cpp Allocator.hpp Proto.hpp Protection.hpp Process.hpp Bytecode.hpp Instructions.hpp Stack.hpp
	$(CXX) $(CXXFLAGS) Allocator.cpp

Unicode.o:	Unicode.cpp Unicode.hpp pl_Unidata.h
	$(CXX) $(CXXFLAGS) Unicode.cpp

Args.o:	Args.cpp Args.hpp
	$(CXX) $(CXXFLAGS) Args.cpp

Assembler.o:	Assembler.cpp Assembler.hpp Instructions.hpp Base.hpp
	$(CXX) $(CXXFLAGS) Assembler.cpp

pl_Unidata.o:	pl_Unidata.c pl_Unidata.h
	$(CC) $(CCFLAGS) pl_Unidata.c -o pl_Unidata.o

pl_Unidata.c:	../misc/unicode_data.pl ../misc/uni/UnicodeData.txt
	perl ../misc/unicode_data.pl source >pl_Unidata.c

pl_Unidata.h:	../misc/unicode_data.pl ../misc/uni/UnicodeData.txt
	perl ../misc/unicode_data.pl header >pl_Unidata.h

Operator.o:	Operator.cpp Operator.h Unicode.hpp pl_Unidata.h
	$(CXX) $(CXXFLAGS) Operator.cpp

Optimizer.o:	Optimizer.cpp Optimizer.hpp Instructions.hpp Symbol.hpp
	$(CXX) $(CXXFLAGS) Optimizer.cpp

CUnicode.o:	CUnicode.cpp CUnicode.h
	$(CXX) $(CXXFLAGS) CUnicode.cpp

Dump.o:	Dump.cpp Dump.hpp Proto.hpp Bytecode.hpp Instructions.hpp
	$(CXX) $(CXXFLAGS) Dump.cpp

Protection.o:	Protection.cpp Protection.hpp
	$(CXX) $(CXXFLAGS) Protection.cpp

Parents.o:	Parents.cpp Parents.hpp Proto.hpp Symbol.hpp
	$(CXX) $(CXXFLAGS) Parents.cpp

Precedence.o:	Precedence.cpp Precedence.hpp Proto.hpp Symbol.hpp Parser.tab.c
	$(CXX) $(CXXFLAGS) Precedence.cpp

Input.o:	Input.cpp Input.hpp
	$(CXX) $(CXXFLAGS) Input.cpp

Base.o:	Base.cpp Base.hpp
	$(CXX) $(CXXFLAGS) Base.cpp

Statics.o:	Statics.cpp Allocator.hpp GC.hpp Proto.hpp
	$(CXX) $(CXXFLAGS) Statics.cpp

main.o:	main.cpp lex.yy.h Standard.hpp Reader.hpp Garnish.hpp GC.hpp REPL.hpp Bytecode.hpp Instructions.hpp Proto.hpp Stack.hpp Args.hpp Pathname.hpp Protection.hpp
	$(CXX) $(CXXFLAGS) main.cpp
