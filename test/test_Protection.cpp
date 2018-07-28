//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "Protection.hpp"
#include <string>

TEST_CASE( "& and | on Protection are idempotent", "[protection]" ) {

  REQUIRE( (Protection::NO_PROTECTION  & Protection::NO_PROTECTION)  == Protection::NO_PROTECTION  );
  REQUIRE( (Protection::PROTECT_ASSIGN & Protection::PROTECT_ASSIGN) == Protection::PROTECT_ASSIGN );
  REQUIRE( (Protection::PROTECT_DELETE & Protection::PROTECT_DELETE) == Protection::PROTECT_DELETE );

  REQUIRE( (Protection::NO_PROTECTION  | Protection::NO_PROTECTION)  == Protection::NO_PROTECTION  );
  REQUIRE( (Protection::PROTECT_ASSIGN | Protection::PROTECT_ASSIGN) == Protection::PROTECT_ASSIGN );
  REQUIRE( (Protection::PROTECT_DELETE | Protection::PROTECT_DELETE) == Protection::PROTECT_DELETE );

}

TEST_CASE( "NO_PROTECTION is the identity for |", "[protection]" ) {

  REQUIRE( (Protection::NO_PROTECTION | Protection::PROTECT_ASSIGN) == Protection::PROTECT_ASSIGN );
  REQUIRE( (Protection::NO_PROTECTION | Protection::PROTECT_DELETE) == Protection::PROTECT_DELETE );
  REQUIRE( (Protection::PROTECT_ASSIGN | Protection::NO_PROTECTION) == Protection::PROTECT_ASSIGN );
  REQUIRE( (Protection::PROTECT_DELETE | Protection::NO_PROTECTION) == Protection::PROTECT_DELETE );

}

TEST_CASE( "NO_PROTECTION is the zero for &", "[protection]" ) {

  REQUIRE( (Protection::NO_PROTECTION & Protection::PROTECT_ASSIGN) == Protection::NO_PROTECTION );
  REQUIRE( (Protection::NO_PROTECTION & Protection::PROTECT_DELETE) == Protection::NO_PROTECTION );
  REQUIRE( (Protection::PROTECT_ASSIGN & Protection::NO_PROTECTION) == Protection::NO_PROTECTION );
  REQUIRE( (Protection::PROTECT_DELETE & Protection::NO_PROTECTION) == Protection::NO_PROTECTION );

}
