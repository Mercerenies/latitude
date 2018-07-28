//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "Stack.hpp"

// We're going to define a non-copyable type here, since StackNode
// shouldn't need to copy.
struct NoCopy {
  int value;

  NoCopy(int n) : value(n) {}
  NoCopy(const NoCopy&) = delete;
  NoCopy(NoCopy&&) = default;

};

TEST_CASE( "Stack nodes", "" ) {

  NodePtr<NoCopy> empty = nullptr;
  NodePtr<NoCopy> once = pushNode(empty, std::move(NoCopy(10)));
  NodePtr<NoCopy> branch1 = pushNode(once, std::move(NoCopy(50)));
  NodePtr<NoCopy> branch2 = pushNode(once, std::move(NoCopy(-10)));

  REQUIRE( branch1->get().value == 50 );
  REQUIRE( branch2->get().value == -10 );
  REQUIRE( once->get().value == 10 );

  REQUIRE( popNode(branch1) == once );
  REQUIRE( popNode(branch2) == once );
  REQUIRE( popNode(once) == empty );

}

TEST_CASE( "Copy initialization of elements in stack nodes", "" ) {

  NodePtr<int> empty = nullptr;
  NodePtr<int> next  = pushNode(empty, 46);
  NodePtr<int> next1 = pushNode(next , 47);

  REQUIRE( next ->get() == 46 );
  REQUIRE( next1->get() == 47 );

  REQUIRE( popNode(next ) == empty );
  REQUIRE( popNode(next1) == next  );

}
