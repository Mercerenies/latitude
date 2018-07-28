//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "Optimizer.hpp"
#include "Assembler.hpp"
#include <string>

TEST_CASE( "lookupSymbols", "[optimize]" ) {

  // lookupSymbols should modify only standard and natural symbols,
  // NOT generated ones
  AssemblerLine sym1 = makeAssemblerLine(Instr::SYM, "some-symbol-name");
  AssemblerLine sym2 = makeAssemblerLine(Instr::SYM, "~generated");
  AssemblerLine sym3 = makeAssemblerLine(Instr::SYM, "10");
  AssemblerLine mov = makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET);
  InstrSeq seq = asmCode(sym1, sym2, sym3, mov);
  TranslationUnitPtr test = std::make_shared<TranslationUnit>( seq );

  test->pushMethod(seq);

  REQUIRE( test->instructions() == seq );
  REQUIRE( test->method(1) == seq );

  optimize::lookupSymbols(test);

  REQUIRE( test->instructions()[0].getCommand() == Instr::SYMN );
  REQUIRE( test->instructions()[1] == sym2 );
  REQUIRE( test->instructions()[2].getCommand() == Instr::SYMN);
  REQUIRE( test->instructions()[3] == mov  );

  REQUIRE( test->method(1)[0].getCommand() == Instr::SYMN );
  REQUIRE( test->method(1)[1] == sym2 );
  REQUIRE( test->method(1)[2].getCommand() == Instr::SYMN );
  REQUIRE( test->method(1)[3] == mov  );

}
