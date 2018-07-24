
LOCAL_FILES=main.o test_Symbol.o

PROJ_FILES=$(addprefix ../src/,$(subst main.o,,$(OBJFILES)))

FILES=$(LOCAL_FILES) $(PROJ_FILES)

CXXFLAGS += -I ../include/ -I ../src/

all:	Testing

test:	Testing

Testing:	$(LOCAL_FILES)
	$(LINK) -o test.out $(FILES)

main.o:	main.cpp
	$(CXX) $(CXXFLAGS) main.cpp
