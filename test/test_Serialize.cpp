//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "Serialize.hpp"
#include <vector>
#include <iterator>

struct Dummy {
  char ch;
};

bool operator==(Dummy a, Dummy b) {
  return a.ch == b.ch;
}

namespace Catch {

  template <>
  struct StringMaker<Dummy> {
    static std::string convert(const Dummy& value) {
      return std::string(1, value.ch);
    }
  };

}

template <>
struct serialize_t<Dummy> {

  using type = Dummy;

  template <typename OutputIterator>
  void serialize(type arg, OutputIterator& iter) const {
    *iter++ = arg.ch;
  }

  template <typename InputIterator>
  type deserialize(InputIterator& iter) const {
    Dummy d { *iter };
    ++iter;
    return d;
  }

};

template <typename T>
T roundTrip(const T& arg) {
  std::vector<unsigned char> vec;
  auto inserter = std::back_inserter(vec);
  serialize(arg, inserter);
  auto res = vec.begin();
  return deserialize<T>(res);
}

TEST_CASE( "Serializing longs", "[serialize]" ) {

  REQUIRE( roundTrip( 10l) ==  10l );
  REQUIRE( roundTrip(999l) == 999l );
  REQUIRE( roundTrip(-20l) == -20l );
  REQUIRE( roundTrip(  0l) ==   0l );

}

TEST_CASE( "Serializing characters", "[serialize]" ) {

  REQUIRE( roundTrip('a' ) == 'a'  );
  REQUIRE( roundTrip('9' ) == '9'  );
  REQUIRE( roundTrip('.' ) == '.'  );
  REQUIRE( roundTrip('\0') == '\0' );

  REQUIRE( roundTrip((char)-1) == (char)-1 );

}

TEST_CASE( "Serializing strings", "[serialize]" ) {

  REQUIRE( roundTrip(std::string("foobar")) == "foobar" );
  REQUIRE( roundTrip(std::string("")) == "" );
  REQUIRE( roundTrip(std::string("...")) == "..." );

  std::string withNulls(4, 'Z');
  withNulls[0] = '\0';
  withNulls[1] = '.';
  withNulls[2] = '\0';
  withNulls[3] = 'Z';
  REQUIRE( roundTrip(withNulls) == withNulls );

}

TEST_CASE( "Serializing Reg", "[serialize]" ) {

  REQUIRE( roundTrip(Reg::PTR) == Reg::PTR );
  REQUIRE( roundTrip(Reg::HAND) == Reg::HAND );
  REQUIRE( roundTrip(Reg::MTHD) == Reg::MTHD );
  REQUIRE( roundTrip(Reg::CONT) == Reg::CONT );

}

TEST_CASE( "Serializing Instr", "[serialize]" ) {

  REQUIRE( roundTrip(Instr::MOV) == Instr::MOV );
  REQUIRE( roundTrip(Instr::XXX) == Instr::XXX );
  REQUIRE( roundTrip(Instr::ECLR) == Instr::ECLR );

}

TEST_CASE( "Serializing FunctionIndex", "[serialize]" ) {

  REQUIRE( roundTrip(FunctionIndex{0}) == FunctionIndex{0} );
  REQUIRE( roundTrip(FunctionIndex{2}) == FunctionIndex{2} );

}

TEST_CASE( "Serializing AssemblerLine", "[serialize]" ) {

  REQUIRE( roundTrip(makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)) ==
           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF) );
  REQUIRE( roundTrip(makeAssemblerLine(Instr::EXPD, Reg::STR1)) ==
           makeAssemblerLine(Instr::EXPD, Reg::STR1) );
  REQUIRE( roundTrip(makeAssemblerLine(Instr::LOAD, Reg::STR1)) ==
           makeAssemblerLine(Instr::LOAD, Reg::STR1) );
  REQUIRE( roundTrip(makeAssemblerLine(Instr::XCALL)) ==
           makeAssemblerLine(Instr::XCALL) );

}

TEST_CASE( "Serializing our dummy type", "[serialize]" ) {

}

// Headers can be serialized and should be tested either here or in
// the test_Header
