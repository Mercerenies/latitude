//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "test.hpp"
#include "Allocator.hpp"
#include "GC.hpp"

TEST_CASE( "The garbage collector can be run by force", "" ) {

  long count = GC::get().garbageCollect(*globalVM);
  REQUIRE( count >= 0 );

}

TEST_CASE( "The garbage collector cleans up cyclic references", "" ) {

  ObjectPtr obj = clone(globalVM->reader.lit[Lit::OBJECT]);
  Object* raw = obj;
  ObjectEntry* data = reinterpret_cast<ObjectEntry*>(raw);

  obj->put(Symbols::get()["cyclicReference"], obj);
  obj = nullptr;

  // The object still retains a cyclic reference to itself, so it
  // can't be cleaned up by reference counting alone.
  REQUIRE( data->in_use );

  // However, it can be cleaned up by the GC
  long count = GC::get().garbageCollect(*globalVM);
  REQUIRE( count >= 1 );
  REQUIRE( !data->in_use );

}
