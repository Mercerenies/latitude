
CCFLAGS=-c -Wall
CXXFLAGS=$(BOOST) -c -Wall -std=gnu++1y
LINKFLAGS=$(BOOST) -Wall -std=gnu++1y
LINK=$(CXX) $(LINKFLAGS) -Wall -std=gnu++1y -o ../latitude

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

export BOOST LINK LINKFLAGS CC CCFLAGS CXX CXXFLAGS

install:
	./latitude --compile
	mkdir -p /usr/local/lib/latitude
	mkdir -p /usr/local/lib/latitude/std
	cp latitude    /usr/local/lib/latitude/
	cp std/*.lat   /usr/local/lib/latitude/std/
	cp std/*.lats  /usr/local/lib/latitude/std/
	cp std/*.latc  /usr/local/lib/latitude/std/
	cp std/*.latsc /usr/local/lib/latitude/std/
	ln -sf /usr/local/lib/latitude/latitude /usr/local/bin/latitude

Project:
	$(MAKE) -C src/

clean:
	rm src/*.o
	rm src/lex.yy.c src/lex.yy.h src/Parser.tab.c src/Parser.tab.h
