//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "test.hpp"
#include "Garnish.hpp"
#include "Proto.hpp"
#include "Number.hpp"
#include "Symbol.hpp"
#include <boost/blank.hpp>
#include <string>

// Not testing the garnishSeq stuff here right now.

TEST_CASE( "String garnishing", "[garnish]" ) {

  ObjectPtr obj = garnishObject(globalVM->reader, std::string("abc"));
  std::string* str = boost::get<std::string>(&obj->prim());

  REQUIRE( str != nullptr );
  REQUIRE( *str == "abc" );
  REQUIRE( (*obj)[Symbols::parent()] == globalVM->reader.lit[Lit::STRING] );

}

TEST_CASE( "Number garnishing", "[garnish]" ) {

  ObjectPtr n1 = garnishObject(globalVM->reader, 10);
  ObjectPtr n2 = garnishObject(globalVM->reader, 10l);
  ObjectPtr n3 = garnishObject(globalVM->reader, Number((Number::smallint)10));

  Number* x1 = boost::get<Number>(&n1->prim());
  Number* x2 = boost::get<Number>(&n2->prim());
  Number* x3 = boost::get<Number>(&n3->prim());

  REQUIRE( x1 != nullptr );
  REQUIRE( x2 != nullptr );
  REQUIRE( x3 != nullptr );

  REQUIRE( *x1 == 10l );
  REQUIRE( *x2 == 10l );
  REQUIRE( *x3 == 10l );

  REQUIRE( (*n1)[Symbols::parent()] == globalVM->reader.lit[Lit::NUMBER] );
  REQUIRE( (*n2)[Symbols::parent()] == globalVM->reader.lit[Lit::NUMBER] );
  REQUIRE( (*n3)[Symbols::parent()] == globalVM->reader.lit[Lit::NUMBER] );

}


TEST_CASE( "Symbol garnishing", "[garnish]" ) {

  Symbolic sym = Symbols::get()["foo"];

  ObjectPtr obj = garnishObject(globalVM->reader, sym);
  Symbolic* sym1 = boost::get<Symbolic>(&obj->prim());

  REQUIRE( sym1 != nullptr );
  REQUIRE( *sym1 == sym );
  REQUIRE( (*obj)[Symbols::parent()] == globalVM->reader.lit[Lit::SYMBOL] );

}

TEST_CASE( "Singleton garnishing (Booleans and nil)", "[garnish]" ) {

  ObjectPtr t = garnishObject(globalVM->reader, true);
  ObjectPtr f = garnishObject(globalVM->reader, false);
  ObjectPtr n = garnishObject(globalVM->reader, boost::blank());

  REQUIRE( t == globalVM->reader.lit[Lit::TRUE] );
  REQUIRE( f == globalVM->reader.lit[Lit::FALSE] );
  REQUIRE( n == globalVM->reader.lit[Lit::NIL] );

}
