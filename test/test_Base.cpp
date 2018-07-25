//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "Base.hpp"
#include <string>

TEST_CASE( "Error types", "" ) {

  std::string sample("Sample text");
  LatitudeError error(sample);

  REQUIRE( error.what() == sample );
  REQUIRE( error.getMessage() == sample );

}
