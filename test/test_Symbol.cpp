//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "Symbol.hpp"
#include <string>

TEST_CASE( "Generated symbols", "[symbol]" ) {

  Symbolic unprefixed = Symbols::gensym();
  Symbolic prefixed = Symbols::gensym("PREFIX");

  SECTION( "Generated symbols are, in fact, generated" ) {
    REQUIRE( Symbols::symbolType(unprefixed) == SymbolType::GENERATED );
    REQUIRE( Symbols::symbolType(  prefixed) == SymbolType::GENERATED );
  }

  SECTION( "Generated symbol names start with ~" ) {
    REQUIRE( Symbols::get()[unprefixed][0] == '~' );
    REQUIRE( Symbols::get()[  prefixed][0] == '~' );
  }

  SECTION( "Generated symbols are unique" ) {
    Symbolic newSymbol = Symbols::gensym("PREFIX");

    REQUIRE( unprefixed != newSymbol );
    REQUIRE(   prefixed != newSymbol );

  }

  SECTION( "The prefix argument is respected" ) {
    std::string name { Symbols::get()[prefixed] };

    REQUIRE( name.substr(1, 6) == "PREFIX" );

  }

  SECTION( "Distinct gensyms have distinct names" ) {
    Symbolic newSymbol = Symbols::gensym();
    REQUIRE( Symbols::get()[newSymbol] != Symbols::get()[unprefixed] );
    REQUIRE( Symbols::get()[newSymbol] != Symbols::get()[  prefixed] );
  }

  SECTION( "Gensyms can be implicitly constructed" ) {
    Symbolic gen1 = Symbols::get()["~FOO"];
    Symbolic gen2 = Symbols::get()["~FOO"];

    REQUIRE( Symbols::symbolType(gen1) == SymbolType::GENERATED );
    REQUIRE( Symbols::symbolType(gen2) == SymbolType::GENERATED );
    REQUIRE( gen1 != gen2 );

  }

}

TEST_CASE( "Natural symbols", "[symbol]" ) {

  Symbolic zero  = Symbols::natural(0);
  Symbolic one   = Symbols::natural(1);
  Symbolic large = Symbols::natural(99999);

  SECTION( "The same natural symbol is equal to itself" ) {
    Symbolic x = Symbols::natural(0);
    REQUIRE( x == zero  );
    REQUIRE( x != one   );
    REQUIRE( x != large );
  }

  SECTION( "Natural symbols have type NATURAL" ) {
    REQUIRE( Symbols::symbolType(zero ) == SymbolType::NATURAL );
    REQUIRE( Symbols::symbolType(one  ) == SymbolType::NATURAL );
    REQUIRE( Symbols::symbolType(large) == SymbolType::NATURAL );
  }

  SECTION( "Natural symbols stringify as numbers" ) {
    REQUIRE( Symbols::get()[zero ] == "0" );
    REQUIRE( Symbols::get()[one  ] == "1" );
    REQUIRE( Symbols::get()[large] == "99999" );
  }

  SECTION( "Natural symbols can be implicitly constructed" ) {
    Symbolic zeroImpl = Symbols::get()["0"];
    REQUIRE( Symbols::symbolType(zeroImpl) == SymbolType::NATURAL );
    REQUIRE( zeroImpl == zero );
  }

}

TEST_CASE( "Standard symbols", "[symbol]" ) {

  Symbolic standard = Symbols::get()["standard"];
  Symbolic parent = Symbols::get()["parent"];

  SECTION( "Standard symbols have type STANDARD" ) {
    REQUIRE( Symbols::symbolType(standard) == SymbolType::STANDARD );
    REQUIRE( Symbols::symbolType(parent) == SymbolType::STANDARD );
  }

  SECTION( "Standard symbols are equal to themselves" ) {
    REQUIRE( standard == Symbols::get()["standard"] );
    REQUIRE( parent == Symbols::get()["parent"] );
  }

  SECTION( "Standard symbols stringify to their name" ) {
    REQUIRE( Symbols::get()[standard] == "standard" );
    REQUIRE( Symbols::get()[parent]   == "parent"   );
  }

}

TEST_CASE( "parent symbol", "[symbol]" ) {

  Symbolic parent = Symbols::parent();

  REQUIRE( parent == Symbols::get()["parent"] );
  REQUIRE( Symbols::symbolType(parent) == SymbolType::STANDARD );

}

TEST_CASE( "operator[] on Symbols is a one-sided inverse", "[symbol]" ) {

  REQUIRE( Symbols::get()[ Symbols::get()["standard-name"] ] == "standard-name" );
  REQUIRE( Symbols::get()[ Symbols::get()["~GENERATEDNAME"] ] == "~GENERATEDNAME" );
  REQUIRE( Symbols::get()[ Symbols::get()["998"] ] == "998" );
  REQUIRE( Symbols::get()[ Symbols::get()["0003"] ] == "3" );

}

TEST_CASE( "==, <, and std::hash on Symbolic is consistent", "[symbol]" ) {

  Symbolic value1 = Symbols::get()["foo"];
  Symbolic value2 = Symbols::get()["bar"];

  SECTION( "== on Symbolic is consistent" ) {
    REQUIRE( value1 != value2 );
    REQUIRE( value1 == value1 );
    REQUIRE( value2 == value2 );
  }

  SECTION( "< on Symbolic is consistent" ) {
    REQUIRE(( (value1 < value2) || (value2 < value1) ));
    REQUIRE( !(value1 < value1) );
    REQUIRE( !(value2 < value2) );
  }

  SECTION( "std::hash on Symbolic is consistent" ) {
    std::hash<Symbolic> hash;
    REQUIRE( hash(value1) == hash(value1) );
    REQUIRE( hash(value2) == hash(value2) );
  }

}
