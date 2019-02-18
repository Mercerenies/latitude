//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "test.hpp"
#include "Allocator.hpp"
#include "GC.hpp"

TEST_CASE( "Allocation and deallocation work as expected", "" ) {

  // Note that we're testing the allocator here, but allocation needs
  // to go through the garbage collector for bookkeeping purposes.
  // Messy? Yes.

  ObjectPtr newObject = GC::get().allocate();

  REQUIRE( newObject != nullptr );

  // Deallocates by ref count

}

/*
TEST_CASE( "Deallocation by ref count actually happens", "" ) {

  ObjectPtr newObject0 = GC::get().allocate();

  REQUIRE( newObject0 != nullptr );

  Object* raw = newObject0.get();
  REQUIRE( reinterpret_cast<ObjectEntry*>(raw)->in_use == true );

  {

    // We're going to force the variable to go out of scope inside
    // this block.

    ObjectPtr stable = newObject0; // Ref count = 2
    newObject0 = nullptr;          // Ref count = 1
  }                                // Ref count = 0

  REQUIRE( reinterpret_cast<ObjectEntry*>(raw)->in_use == false );

}
*/
