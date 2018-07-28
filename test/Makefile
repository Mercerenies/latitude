
LOCAL_FILES=main.o test_Symbol.o test_Number.o test_Base.o test_Macro.o test_Args.o test_Garnish.o test_Instructions.o test_Optimizer.o test_Parents.o test_Stack.o

PROJ_FILES=$(addprefix ../src/,$(subst main.o,,$(OBJFILES)))

FILES=$(LOCAL_FILES) $(PROJ_FILES)

CXXFLAGS += -I ../include/ -I ../src/

all:	Testing

test:	Testing

test-clean:
	rm *.o

Testing:	$(LOCAL_FILES)
	$(LINK) -o test.out $(FILES)

main.o:	main.cpp test.hpp
	$(CXX) $(CXXFLAGS) main.cpp
