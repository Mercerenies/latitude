//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "test.hpp"
#include "HashMap.hpp"

TEST_CASE( "Size of maps", "" ) {

  HashMap<int, int> map;
  REQUIRE( map.size() == 0 );

  map.put(1, 50);
  REQUIRE( map.size() == 1 );

  map.put(2, 100);
  REQUIRE( map.size() == 2 );

  map.put(1, 150);
  REQUIRE( map.size() == 2 );

}

TEST_CASE( "Maps actually store information", "" ) {

  HashMap<int, int> map;
  REQUIRE( map.get(1) == nullptr );

  map.put(1, 50);
  REQUIRE( *map.get(1) == 50 );

  map.put(1, -50);
  REQUIRE( *map.get(1) == -50 );

  map.put(2, 999);
  REQUIRE( *map.get(1) == -50 );

}

TEST_CASE( "Maps can rehash", "" ) {

  HashMap<int, int> map;
  for (int i = 1; i < 9999; i++)
    map.put(i, i);

  // Probably had a rehash by now.
  REQUIRE( *map.get(765) == 765 );
  REQUIRE( *map.get(53) == 53 );
}
