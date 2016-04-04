
ifndef BOOST
  BOOST='D:/boost/boost_1_60_0'
endif

C=gcc -c -Wall
CC=g++ -I $(BOOST) -c -Wall -std=c++14
LINK=g++ -Wall -std=c++14
FILES=Proto.o Standard.o Scanner.o Parser.o main.o Reader.o Stream.o Garnish.o GC.o Symbol.o Cont.o

all:	$(FILES)
	$(LINK) $(FILES)

clean:
	rm *.o
	rm lex.yy.c lex.yy.h Parser.tab.c Parser.tab.h

Proto.o:	Proto.cpp Proto.hpp Stream.hpp GC.hpp Symbol.hpp Cont.hpp
	$(CC) Proto.cpp

Standard.o:	Standard.cpp Standard.hpp Proto.hpp Reader.hpp Stream.hpp Garnish.hpp Macro.hpp Parser.tab.c GC.hpp Cont.hpp
	$(CC) Standard.cpp

Scanner.o:	lex.yy.c lex.yy.h
	$(C) lex.yy.c -o Scanner.o

Parser.o:	Parser.tab.c
	$(CC) Parser.tab.c -o Parser.o

lex.yy.c:	Scanner.flex Parser.tab.c
	flex Scanner.flex

Parser.tab.c:	Parser.y
	bison -d Parser.y

Reader.o:	Reader.cpp Reader.hpp Parser.tab.c Symbol.hpp
	$(CC) Reader.cpp

Stream.o:	Stream.cpp Stream.hpp
	$(CC) Stream.cpp

Garnish.o:	Garnish.cpp Garnish.hpp Proto.hpp Stream.hpp Reader.hpp
	$(CC) Garnish.cpp

GC.o:	GC.cpp GC.hpp Proto.hpp
	$(CC) GC.cpp

Symbol.o:	Symbol.cpp Symbol.hpp
	$(CC) Symbol.cpp

Cont.o:	Cont.cpp Cont.hpp Symbol.hpp Proto.hpp
	$(CC) Cont.cpp

main.o:	main.cpp lex.yy.h Standard.hpp Reader.hpp Garnish.hpp GC.hpp
	$(CC) main.cpp
