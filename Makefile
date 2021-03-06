
OBJFILES=Proto.o Standard.o Scanner.o Parser.o main.o Reader.o Stream.o Garnish.o GC.o Symbol.o REPL.o Number.o Process.o Bytecode.o Header.o Instructions.o Environment.o Pathname.o Allocator.o Unicode.o Args.o Assembler.o pl_Unidata.o Operator.o Optimizer.o CUnicode.o Protection.o Dump.o Parents.o Precedence.o Input.o Base.o Statics.o

CCFLAGS=-c -std=c99 -Wall
CXXFLAGS=$(BOOST) -c -Wall -std=gnu++1y
LINKFLAGS=$(BOOST) -Wall -std=gnu++1y
LINK=$(CXX) $(LINKFLAGS) -Wall -std=gnu++1y

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

test:	Project
	$(MAKE) -C test/

test-clean:
	$(MAKE) -C test/ test-clean

export BOOST LINK LINKFLAGS CC CCFLAGS CXX CXXFLAGS OBJFILES

install:
	./latitude --compile
	mkdir -p /usr/local/lib/latitude
	mkdir -p /usr/local/lib/latitude/std
	cp latitude    /usr/local/lib/latitude/
	cp std/*.latc  /usr/local/lib/latitude/std/
	cp std/*.latsc /usr/local/lib/latitude/std/
	ln -sf /usr/local/lib/latitude/latitude /usr/local/bin/latitude

Project:
	$(MAKE) -C src/

clean:
	rm src/*.o
	rm src/lex.yy.c src/lex.yy.h src/Parser.tab.c src/Parser.tab.h
