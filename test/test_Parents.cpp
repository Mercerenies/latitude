//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "test.hpp"
#include "Parents.hpp"
#include "Instructions.hpp"
#include "Garnish.hpp"

TEST_CASE( "origin() and objectGet()", "" ) {

  ObjectPtr sentinel1 = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr sentinel2 = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr sentinel3 = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr sentinel4 = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr sentinel5 = clone(globalVM->reader.lit[Lit::OBJECT]);

  ObjectPtr base = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr derived = clone(base);
  ObjectPtr child = clone(derived);

  base->put(Symbols::get()["baseSlot"], sentinel1);
  base->put(Symbols::get()["overriddenSlot"], sentinel2);

  derived->put(Symbols::get()["overriddenSlot"], sentinel3);
  derived->put(Symbols::get()["derivedSlot"], sentinel4);

  child->put(Symbols::get()["childSlot"], sentinel5);

  SECTION( "origin() calls" ) {

    SECTION( "If the slot exists directly" ) {
      REQUIRE( origin(base, Symbols::get()["baseSlot"]) == base );
      REQUIRE( origin(base, Symbols::get()["overriddenSlot"]) == base );
      REQUIRE( origin(derived, Symbols::get()["overriddenSlot"]) == derived );
      REQUIRE( origin(derived, Symbols::get()["derivedSlot"]) == derived );
      REQUIRE( origin(child, Symbols::get()["childSlot"]) == child );
    }

    SECTION( "If the slot exists indirectly" ) {

      REQUIRE( origin(derived, Symbols::get()["baseSlot"]) == base );
      REQUIRE( origin(child, Symbols::get()["baseSlot"]) == base );

      REQUIRE( origin(child, Symbols::get()["overriddenSlot"]) == derived );
      REQUIRE( origin(child, Symbols::get()["derivedSlot"]) == derived );
    }

    SECTION( "If the slot fails to exist" ) {

      REQUIRE( origin(base, Symbols::get()["nonexistentSlot"]) == nullptr );
      REQUIRE( origin(derived, Symbols::get()["nonexistentSlot"]) == nullptr );
      REQUIRE( origin(child, Symbols::get()["nonexistentSlot"]) == nullptr );

      REQUIRE( origin(base, Symbols::get()["derivedSlot"]) == nullptr );
      REQUIRE( origin(base, Symbols::get()["childSlot"]) == nullptr );

      REQUIRE( origin(derived, Symbols::get()["childSlot"]) == nullptr );

    }

  }

  SECTION( "objectGet() calls" ) {

    SECTION( "If the slot exists directly" ) {
      REQUIRE( objectGet(base, Symbols::get()["baseSlot"]) == sentinel1 );
      REQUIRE( objectGet(base, Symbols::get()["overriddenSlot"]) == sentinel2 );
      REQUIRE( objectGet(derived, Symbols::get()["overriddenSlot"]) == sentinel3 );
      REQUIRE( objectGet(derived, Symbols::get()["derivedSlot"]) == sentinel4 );
      REQUIRE( objectGet(child, Symbols::get()["childSlot"]) == sentinel5 );
    }

    SECTION( "If the slot exists indirectly" ) {

      REQUIRE( objectGet(derived, Symbols::get()["baseSlot"]) == sentinel1 );
      REQUIRE( objectGet(child, Symbols::get()["baseSlot"]) == sentinel1 );

      REQUIRE( objectGet(child, Symbols::get()["overriddenSlot"]) == sentinel3 );
      REQUIRE( objectGet(child, Symbols::get()["derivedSlot"]) == sentinel4 );
    }

    SECTION( "If the slot fails to exist" ) {

      REQUIRE( objectGet(base, Symbols::get()["nonexistentSlot"]) == nullptr );
      REQUIRE( objectGet(derived, Symbols::get()["nonexistentSlot"]) == nullptr );
      REQUIRE( objectGet(child, Symbols::get()["nonexistentSlot"]) == nullptr );

      REQUIRE( objectGet(base, Symbols::get()["derivedSlot"]) == nullptr );
      REQUIRE( objectGet(base, Symbols::get()["childSlot"]) == nullptr );

      REQUIRE( objectGet(derived, Symbols::get()["childSlot"]) == nullptr );

    }

  }

}
