//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "test.hpp"
#include "Proto.hpp"
#include "Protection.hpp"
#include "GC.hpp"
#include <boost/blank.hpp>

TEST_CASE( "Accessing slots", "" ) {

  ObjectPtr obj = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr inner = clone(globalVM->reader.lit[Lit::OBJECT]);

  obj->put(Symbols::get()["foobar"], inner);
  REQUIRE( (*obj)[ Symbols::get()["foobar"] ] == inner );
  REQUIRE( (*obj)[ Symbols::get()["nobody"] ] == nullptr );
  REQUIRE( (*obj)[ Symbols::get()["parent"] ] == globalVM->reader.lit[Lit::OBJECT] );

  std::set<Symbolic> keys { Symbols::get()["foobar"], Symbols::parent() };
  REQUIRE( obj->directKeys() == keys );

  obj->remove(Symbols::get()["foobar"]);

  REQUIRE( (*obj)[ Symbols::get()["foobar"] ] == nullptr );

  REQUIRE( obj->directKeys() == std::set<Symbolic> { Symbols::parent() } );

}

TEST_CASE( "Object protection", "" ) {

  ObjectPtr obj = clone(globalVM->reader.lit[Lit::OBJECT]);

  Symbolic a = Symbols::get()["a"];
  Symbolic b = Symbols::get()["b"];
  Symbolic c = Symbols::get()["c"];
  Symbolic z = Symbols::get()["z"];

  obj->put(a, obj);
  obj->put(b, obj);
  obj->put(c, obj);

  REQUIRE(  obj->addProtection(a, Protection::PROTECT_ASSIGN) );
  REQUIRE(  obj->addProtection(b, Protection::PROTECT_ASSIGN | Protection::PROTECT_DELETE) );
  REQUIRE( !obj->addProtection(z, Protection::PROTECT_DELETE) );

  REQUIRE(  obj->hasAnyProtection(a) );
  REQUIRE(  obj->hasAnyProtection(b) );
  REQUIRE( !obj->hasAnyProtection(c) );
  REQUIRE( !obj->hasAnyProtection(z) );

  REQUIRE(  obj->isProtected(a, Protection::PROTECT_ASSIGN) );
  REQUIRE(  obj->isProtected(b, Protection::PROTECT_ASSIGN) );
  REQUIRE( !obj->isProtected(c, Protection::PROTECT_ASSIGN) );
  REQUIRE( !obj->isProtected(z, Protection::PROTECT_ASSIGN) );

  REQUIRE( !obj->isProtected(a, Protection::PROTECT_DELETE) );
  REQUIRE(  obj->isProtected(b, Protection::PROTECT_DELETE) );
  REQUIRE( !obj->isProtected(c, Protection::PROTECT_DELETE) );
  REQUIRE( !obj->isProtected(z, Protection::PROTECT_DELETE) );

  REQUIRE(  obj->isProtected(a, Protection::NO_PROTECTION) );
  REQUIRE(  obj->isProtected(b, Protection::NO_PROTECTION) );
  REQUIRE(  obj->isProtected(c, Protection::NO_PROTECTION) );
  REQUIRE(  obj->isProtected(z, Protection::NO_PROTECTION) );

  obj->protectAll(Protection::PROTECT_DELETE, a, b, c, z);

  REQUIRE(  obj->isProtected(a, Protection::PROTECT_DELETE) );
  REQUIRE(  obj->isProtected(b, Protection::PROTECT_DELETE) );
  REQUIRE(  obj->isProtected(c, Protection::PROTECT_DELETE) );
  REQUIRE( !obj->isProtected(z, Protection::PROTECT_DELETE) );

}

TEST_CASE( "Primitive field", "" ) {

  ObjectPtr obj0 = clone(globalVM->reader.lit[Lit::OBJECT]);

  REQUIRE( (obj0->prim() == Prim( boost::blank() )) );
  REQUIRE( (obj0->prim(10l) == Prim( boost::blank() )) );
  REQUIRE( (obj0->prim() == Prim( 10l )) );

  ObjectPtr obj1 = clone(obj0);

  REQUIRE( (obj1->prim() == Prim( 10l )) );
  REQUIRE( (obj0->prim(std::string("ABC")) == Prim( 10l )) );
  REQUIRE( (obj1->prim() == Prim( 10l )) );
  REQUIRE( (obj0->prim() == Prim( std::string("ABC") )) );

}

TEST_CASE( "Cloning objects", "" ) {

  ObjectPtr obj = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr obj1 = clone(obj);

  REQUIRE( obj  != nullptr );
  REQUIRE( obj1 != nullptr );
  REQUIRE( obj  != obj1    );

}

TEST_CASE( "(Indirect) keys and hierarchy", "" ) {

  ObjectPtr obj0 = GC::get().allocate();
  obj0->put(Symbols::parent(), obj0);

  ObjectPtr obj1 = clone(obj0);

  SECTION( "Indirect keys" ) {

    obj0->put(Symbols::get()["parentSlot"], obj0);
    obj1->put(Symbols::get()["childSlot"], obj1);

    REQUIRE( keys(obj0) == std::set<Symbolic> { Symbols::get()["parentSlot"],
                                                Symbols::parent() } );
    REQUIRE( keys(obj1) == std::set<Symbolic> { Symbols::get()["parentSlot"],
                                                Symbols::get()["childSlot"],
                                                Symbols::parent() } );

  }

  SECTION( "Simple hierarchies" ) {
    REQUIRE( hierarchy(obj0) == std::list<ObjectPtr> { obj0 } );
    REQUIRE( hierarchy(obj1) == std::list<ObjectPtr> { obj1, obj0 } );
  }

  SECTION( "Complicated hierarchies" ) {
    ObjectPtr a = GC::get().allocate();
    ObjectPtr b = clone(a);
    ObjectPtr c = clone(b);
    a->put(Symbols::parent(), c);
    a->addProtection(Symbols::parent(), Protection::PROTECT_DELETE);

    // Now we've created a nontrivial cycle in parents. This isn't
    // actually done in Latitude anywhere right now, but it is
    // supported behavior, strictly speaking, so we need to test it.
    REQUIRE( hierarchy(a) == std::list<ObjectPtr> { a, c, b } );
    REQUIRE( hierarchy(b) == std::list<ObjectPtr> { b, a, c } );
    REQUIRE( hierarchy(c) == std::list<ObjectPtr> { c, b, a } );
  }

}
