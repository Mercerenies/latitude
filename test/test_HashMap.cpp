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

TEST_CASE( "Keys can be deleted", "" ) {

  HashMap<int, int> map;
  for (int i = 1; i < 9999; i++)
    map.put(i, i);

  REQUIRE( map.remove(10) == true  );
  REQUIRE( map.remove(20) == true  );
  REQUIRE( map.remove(20) == false );
  REQUIRE( map.remove(99999) == false );

  REQUIRE( *map.get(4) == 4 );
  REQUIRE( map.remove(4) == true );
  REQUIRE( map.remove(4) == false );
  REQUIRE( map.get(4) == nullptr );

}

TEST_CASE( "Deletion does not break other keys", "" ) {

  HashMap<int, int> map;
  for (int i = 1; i < 9999; i++)
    map.put(i, i);

  REQUIRE( map.remove(10) == true  );
  REQUIRE( map.remove(20) == true  );
  REQUIRE( map.remove(30) == true  );
  REQUIRE( map.remove(35) == true  );
  REQUIRE( map.remove(40) == true  );

  REQUIRE( *map.get(11) == 11 );
  REQUIRE( *map.get(12) == 12 );
  REQUIRE( *map.get(13) == 13 );
  REQUIRE( *map.get(14) == 14 );
  REQUIRE( *map.get(111) == 111 );

}

TEST_CASE( "Deleted slots can be reassigned", "" ) {

  HashMap<int, int> map;

  map.put(10, 1);
  map.put(111, 2);

  REQUIRE( map.remove(10) == true );

  REQUIRE( *map.get(111) == 2 );

}

