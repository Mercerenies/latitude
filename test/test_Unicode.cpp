//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "Unicode.hpp"
#include "pl_Unidata.h"
#include <string>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

TEST_CASE( "Stringification of Unicode produces a single 'character'", "[unicode]" ) {

  UniChar alphaLower { 0x03b1 };
  UniChar alphaUpper { 0x0391 };

  std::string stringLower = alphaLower;
  std::string stringUpper = alphaUpper;

  auto alphaLower1 = charAt(stringLower, 0);
  auto alphaUpper1 = charAt(stringUpper, 0);

  REQUIRE( (bool)alphaLower1 );
  REQUIRE( (bool)alphaUpper1 );

  REQUIRE( alphaLower1->codePoint() == alphaLower.codePoint() );
  REQUIRE( alphaUpper1->codePoint() == alphaUpper.codePoint() );

  REQUIRE( !charAt(stringLower,  1) );
  REQUIRE( !charAt(stringLower,  2) );
  REQUIRE( !charAt(stringLower,  3) );
  REQUIRE( !charAt(stringLower,  4) );
  REQUIRE( !charAt(stringLower,  5) );
  REQUIRE( !charAt(stringLower, -1) );
  REQUIRE( !nextCharPos(stringLower,  2) );
  REQUIRE( !nextCharPos(stringLower, 10) );
  REQUIRE( !nextCharPos(stringLower, -2) );

  REQUIRE( nextCharPos(stringLower, 0) );
  REQUIRE( *nextCharPos(stringLower, 0) == stringLower.length() );

}

TEST_CASE( "General Unicode categories", "[unicode]" ) {

  UniChar lu  { 0x0044 }; // LATIN CAPITAL LETTER D
  UniChar nd  { 0x0030 }; // DIGIT ZERO
  UniChar sm  { 0x2295 }; // CIRCLED PLUS
  UniChar lu1 { 0x2115 }; // DOUBLE-STRUCK CAPITAL N
  UniChar po  { 0x002c }; // COMMA
  UniChar sc  { 0x0024 }; // DOLLAR SIGN

  REQUIRE( lu .genCat() == UNICLS_LU );
  REQUIRE( nd .genCat() == UNICLS_ND );
  REQUIRE( sm .genCat() == UNICLS_SM );
  REQUIRE( lu1.genCat() == UNICLS_LU );
  REQUIRE( po .genCat() == UNICLS_PO );
  REQUIRE( sc .genCat() == UNICLS_SC );

}

TEST_CASE( "Unicode case folding", "[unicode]" ) {

  UniChar alphaLower { 0x03b1 };
  UniChar alphaUpper { 0x0391 };
  UniChar dot        { 0x002e };

  REQUIRE( alphaLower.toUpper().codePoint() == alphaUpper.codePoint() );
  REQUIRE( alphaLower.toTitle().codePoint() == alphaUpper.codePoint() );
  REQUIRE( alphaLower.toLower().codePoint() == alphaLower.codePoint() );

  REQUIRE( alphaUpper.toUpper().codePoint() == alphaUpper.codePoint() );
  REQUIRE( alphaUpper.toTitle().codePoint() == alphaUpper.codePoint() );
  REQUIRE( alphaUpper.toLower().codePoint() == alphaLower.codePoint() );

  REQUIRE( dot.toUpper().codePoint() == dot.codePoint() );
  REQUIRE( dot.toTitle().codePoint() == dot.codePoint() );
  REQUIRE( dot.toLower().codePoint() == dot.codePoint() );

}

TEST_CASE( "charAt and nextCharPos", "[unicode]" ) {

  UniChar oplus { 0x2295 };
  UniChar dot   { 0x002e };

  std::string str = std::string(oplus) + std::string(dot);

  auto firstChar = charAt(str, 0);
  auto secondCharPos = nextCharPos(str, 0);

  REQUIRE( (bool)firstChar );
  REQUIRE( firstChar->codePoint() == oplus.codePoint() );

  REQUIRE( (bool)secondCharPos );

  auto secondChar = charAt(str, *secondCharPos);
  auto thirdCharPos = nextCharPos(str, *secondCharPos);

  REQUIRE( (bool)secondChar );
  REQUIRE( secondChar->codePoint() == dot.codePoint() );

  REQUIRE( (bool)thirdCharPos );
  REQUIRE( *thirdCharPos == str.length() );
  REQUIRE( !nextCharPos(str, *thirdCharPos) );
  REQUIRE( !charAt(str, *thirdCharPos) );

}
