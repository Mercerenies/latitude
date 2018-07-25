//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "Macro.hpp"
#include <boost/variant.hpp>

class VariantSample1 {};
class VariantSample2 {};

TEST_CASE( "variantIsType", "" ) {

  boost::variant<VariantSample1, VariantSample2> var1 { VariantSample1() };
  boost::variant<VariantSample1, VariantSample2> var2 { VariantSample2() };

  REQUIRE(  variantIsType<VariantSample1>(var1) );
  REQUIRE( !variantIsType<VariantSample1>(var2) );
  REQUIRE( !variantIsType<VariantSample2>(var1) );
  REQUIRE(  variantIsType<VariantSample2>(var2) );

}
