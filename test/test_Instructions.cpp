//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "Instructions.hpp"
#include "Assembler.hpp"
#include "Base.hpp"

TEST_CASE( "InstructionSet behavior", "" ) {

  InstructionSet& iset = InstructionSet::getInstance();

  for (unsigned char i = 0x01; i <= (unsigned char)Instr::MSWAP; i++) {
    REQUIRE( iset.hasInstruction((Instr)i) );
  }

  // Could also test getParams here...?

}

TEST_CASE( "Validator predicates", "" ) {

  std::string sample = "foobar";
  FunctionIndex index { 1 };

  SECTION( "isRegister" ) {
    REQUIRE(  isRegister(Reg::PTR ) );
    REQUIRE(  isRegister(Reg::HAND) );
    REQUIRE(  isRegister(Reg::SYM ) );
    REQUIRE( !isRegister(      10l) );
    REQUIRE( !isRegister(   sample) );
    REQUIRE( !isRegister(    index) );
  }

  SECTION( "isObjectRegister" ) {
    REQUIRE(  isObjectRegister(Reg::PTR ) );
    REQUIRE( !isObjectRegister(Reg::HAND) );
    REQUIRE( !isObjectRegister(Reg::SYM ) );
    REQUIRE( !isObjectRegister(      10l) );
    REQUIRE( !isObjectRegister(   sample) );
    REQUIRE( !isObjectRegister(    index) );
  }

  SECTION( "isStackRegister" ) {
    REQUIRE( !isStackRegister(Reg::PTR ) );
    REQUIRE(  isStackRegister(Reg::HAND) );
    REQUIRE( !isStackRegister(Reg::SYM ) );
    REQUIRE( !isStackRegister(      10l) );
    REQUIRE( !isStackRegister(   sample) );
    REQUIRE( !isStackRegister(    index) );
  }

  SECTION( "isStringRegisterArg" ) {
    REQUIRE( !isStringRegisterArg(Reg::PTR ) );
    REQUIRE( !isStringRegisterArg(Reg::HAND) );
    REQUIRE( !isStringRegisterArg(Reg::SYM ) );
    REQUIRE( !isStringRegisterArg(      10l) );
    REQUIRE(  isStringRegisterArg(   sample) );
    REQUIRE( !isStringRegisterArg(    index) );
  }

  SECTION( "isLongRegisterArg" ) {
    REQUIRE( !isLongRegisterArg(Reg::PTR ) );
    REQUIRE( !isLongRegisterArg(Reg::HAND) );
    REQUIRE( !isLongRegisterArg(Reg::SYM ) );
    REQUIRE(  isLongRegisterArg(      10l) );
    REQUIRE( !isLongRegisterArg(   sample) );
    REQUIRE( !isLongRegisterArg(    index) );
  }

  SECTION( "isAsmRegisterArg" ) {
    REQUIRE( !isAsmRegisterArg(Reg::PTR ) );
    REQUIRE( !isAsmRegisterArg(Reg::HAND) );
    REQUIRE( !isAsmRegisterArg(Reg::SYM ) );
    REQUIRE( !isAsmRegisterArg(      10l) );
    REQUIRE( !isAsmRegisterArg(   sample) );
    REQUIRE(  isAsmRegisterArg(    index) );
  }

}

TEST_CASE( "AssemblerLine construction and validation", "" ) {

  SECTION( "AssemblerLine defaults" ) {

    AssemblerLine def;
    AssemblerLine mov { Instr::MOV };

    // REQUIRE( def.getCommand() == ??? );
    REQUIRE( mov.getCommand() == Instr::MOV );
    REQUIRE( def.argumentCount() == 0 );
    REQUIRE( mov.argumentCount() == 0 );

    // REQUIRE_THROWS_AS( def.validate(), AssemblerError );
    REQUIRE_THROWS_AS( mov.validate(), AssemblerError );

  }

  SECTION( "Setting the command" ) {

    AssemblerLine def;

    def.setCommand(Instr::NRET);
    REQUIRE( def.getCommand() == Instr::NRET );

    def.setCommand(Instr::XXX);
    REQUIRE( def.getCommand() == Instr::XXX );

  }

  SECTION( "Arguments" ) {

    AssemblerLine mov { Instr::MOV };

    REQUIRE( mov.argumentCount() == 0 );
    REQUIRE( mov.arguments().begin() == mov.arguments().end() );

    mov.addRegisterArg(10l);
    mov.addRegisterArg(std::string("foobar"));

    REQUIRE( mov.argumentCount() == 2 );
    REQUIRE( (mov.argument(0) == RegisterArg(10l)) );
    REQUIRE( (mov.argument(1) == RegisterArg(std::string("foobar"))) );

    auto start = mov.arguments().begin();
    REQUIRE( (*start == RegisterArg(10l)) );
    ++start;
    REQUIRE( (*start == RegisterArg(std::string("foobar"))) );
    ++start;
    REQUIRE( start == mov.arguments().end() );

    mov.clearRegisterArgs();
    REQUIRE( mov.argumentCount() == 0 );
    REQUIRE( mov.arguments().begin() == mov.arguments().end() );

  }

  SECTION( "Validation of arguments" ) {

    AssemblerLine mov { Instr::MOV };

    REQUIRE_THROWS_AS( mov.validate(), AssemblerError );
    mov.addRegisterArg(Reg::PTR);
    REQUIRE_THROWS_AS( mov.validate(), AssemblerError );
    mov.addRegisterArg(Reg::HAND);
    REQUIRE_THROWS_AS( mov.validate(), AssemblerError );

    mov.clearRegisterArgs();
    mov.addRegisterArg(Reg::PTR);
    mov.addRegisterArg(Reg::SLF);
    mov.validate(); // Should not throw

  }

  SECTION( "Appending onto instruction sequences" ) {

    AssemblerLine push { Instr::PUSH };
    push.addRegisterArg(Reg::SLF);
    push.addRegisterArg(Reg::DYN);

    AssemblerLine pop { Instr::POP };
    pop.addRegisterArg(Reg::SLF);
    pop.addRegisterArg(Reg::DYN);

    InstrSeq seq;
    push.appendOnto(seq);
    pop.appendOnto(seq);
    REQUIRE( (seq == InstrSeq { push, pop }) );

  }

}

TEST_CASE( "Translation units", "" ) {

  SECTION( "Default initialization" ) {
    TranslationUnit def;
    REQUIRE( def.methodCount() == 1 );
    REQUIRE( def.instructions() == InstrSeq() );
    REQUIRE( def.method(0) == InstrSeq {} );
  }

  SECTION( "Adding methods" ) {
    InstrSeq instr = asmCode(makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::PTR));
    TranslationUnit withCode { instr };
    REQUIRE( withCode.methodCount() == 1 );
    REQUIRE( withCode.instructions() == instr );
    REQUIRE( withCode.method(0) == instr );

    InstrSeq instr1 = asmCode(makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::LEX),
                              makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::LEX));
    withCode.pushMethod(instr1);
    REQUIRE( withCode.methodCount() == 2 );
    REQUIRE( withCode.method(0) == instr  );
    REQUIRE( withCode.method(1) == instr1 );

    InstrSeq instr2 = instr1;
    withCode.pushMethod(std::move(instr2));
    REQUIRE( withCode.methodCount() == 3 );
    REQUIRE( withCode.method(0) == instr  );
    REQUIRE( withCode.method(1) == instr1 );
    REQUIRE( withCode.method(2) == instr1 );

  }

}

TEST_CASE( "Methods and MethodSeek", "" ) {

  InstrSeq instr = asmCode(makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::PTR),
                           makeAssemblerLine(Instr::INT, 10l),
                           makeAssemblerLine(Instr::STR, "Dummy instruction"),
                           makeAssemblerLine(Instr::STR, "Literal string"),
                           makeAssemblerLine(Instr::MTHD, FunctionIndex{ 0 }),
                           makeAssemblerLine(Instr::SYM, "dummy-instruction-1"),
                           makeAssemblerLine(Instr::SYM, "dummy-instruction-2"));
  TranslationUnitPtr test = std::make_shared<TranslationUnit>( instr );
  Method mthd { test, { 0 } };

  SECTION( "Method properties" ) {
    REQUIRE( mthd.instructions() == instr );
    REQUIRE( mthd.size() == 7 );
    REQUIRE( mthd.translationUnit() == test );
    REQUIRE( mthd.index() == FunctionIndex { 0 } );
  }

  SECTION( "MethodSeek properties" ) {
    MethodSeek seek { mthd };

    REQUIRE( seek.size() == mthd.size() );
    REQUIRE( seek.instructions() == instr );

    REQUIRE( seek.position() == (unsigned long)-1L );

    seek.advancePosition(1);
    REQUIRE( !seek.atEnd() );
    REQUIRE( seek.position() == 0 );
    REQUIRE( seek.readInstr() == Instr::MOV );
    REQUIRE( seek.readReg(0) == Reg::SLF );
    REQUIRE( seek.readReg(1) == Reg::PTR );

    seek.advancePosition(1);
    REQUIRE( !seek.atEnd() );
    REQUIRE( seek.position() == 1 );
    REQUIRE( seek.readInstr() == Instr::INT );
    REQUIRE( seek.readLong(0) == 10l );

    seek.advancePosition(0);
    REQUIRE( seek.position() == 1 );

    seek.advancePosition(2);
    REQUIRE( !seek.atEnd() );
    REQUIRE( seek.position() == 3 );
    REQUIRE( seek.readInstr() == Instr::STR );
    REQUIRE( seek.readString(0) == "Literal string" );

    seek.advancePosition(1);
    REQUIRE( !seek.atEnd() );
    REQUIRE( seek.position() == 4 );
    REQUIRE( seek.readInstr() == Instr::MTHD );
    REQUIRE( seek.readFunction(0) == FunctionIndex{ 0 } );

    seek.advancePosition(3);
    REQUIRE( seek.atEnd() );
    REQUIRE( seek.position() == 7 );

  }

}
