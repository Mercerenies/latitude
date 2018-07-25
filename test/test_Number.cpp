//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "Number.hpp"
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
#include <string>
#include <complex>
#include <limits>

TEST_CASE( "Number construction", "[number]" ) {
  REQUIRE( Number((Number::smallint)0).hierarchyLevel() == Number::SMALLINT );
  REQUIRE( Number((Number::bigint)10).hierarchyLevel() == Number::BIGINT );
  REQUIRE( Number( Number::ratio(1, 2) ).hierarchyLevel() == Number::RATIO );
  REQUIRE( Number(3.0).hierarchyLevel() == Number::FLOATING );
  REQUIRE( Number(1+1i).hierarchyLevel() == Number::COMPLEX );

  Number a { (Number::smallint)10 };
  Number b = a;

  REQUIRE( a == b );

}

TEST_CASE( "Number equality and comparison", "[number]" ) {

  Number smallZero { (Number::smallint)0 };
  Number   bigZero { (Number::bigint)  0 };

  Number   intOne { (Number::bigint) 1  };
  Number ratioOne { Number::ratio(1, 1) };

  Number floatingOne { 1.0 };

  REQUIRE( smallZero == smallZero );
  REQUIRE(   bigZero ==   bigZero );
  REQUIRE( smallZero ==   bigZero );
  REQUIRE( smallZero !=    intOne );
  REQUIRE(  ratioOne ==    intOne );
  REQUIRE(  ratioOne ==  ratioOne );

  REQUIRE(   smallZero <      intOne );
  REQUIRE(     bigZero <      intOne );
  REQUIRE(   smallZero <    ratioOne );
  REQUIRE(      intOne >   smallZero );
  REQUIRE(     bigZero < floatingOne );
  REQUIRE(         0.0 < floatingOne );
  REQUIRE( floatingOne <         2.0 );

}

TEST_CASE( "Number addition and subtraction", "[number]" ) {

  Number    small { (Number::smallint)1 };
  Number      big { (Number::bigint)  1 };
  Number    ratio { Number::ratio(1, 1) };
  Number floating {                1.0  };
  Number  complex {          1.0 + 0.0i };

  SECTION( "Addition and subtraction behave correctly" ) {
    REQUIRE( small + small == 2l );
    REQUIRE( small +   big == 2l );
    REQUIRE( small + ratio == 2l );
    REQUIRE(   big +   big == 2l );
    REQUIRE(   big + ratio == 2l );
    REQUIRE( ratio + ratio == 2l );
    REQUIRE( small - small == 0l );
    REQUIRE( small -   big == 0l );
    REQUIRE( small - ratio == 0l );
    REQUIRE(   big -   big == 0l );
    REQUIRE(   big - ratio == 0l );
    REQUIRE( ratio - ratio == 0l );
  }

  SECTION( "Negation behaves correctly" ) {
    REQUIRE( -small == -1l );
    REQUIRE( -  big == -1l );
    REQUIRE( -ratio == -1l );
  }

  SECTION( "Addition and subtraction coerce to the larger type" ) {
    REQUIRE( (   small +    small).hierarchyLevel() == Number::SMALLINT );
    REQUIRE( (   small +      big).hierarchyLevel() == Number::BIGINT   );
    REQUIRE( (   small +    ratio).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   small + floating).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   small +  complex).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( (   small -    small).hierarchyLevel() == Number::SMALLINT );
    REQUIRE( (   small -      big).hierarchyLevel() == Number::BIGINT   );
    REQUIRE( (   small -    ratio).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   small - floating).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   small -  complex).hierarchyLevel() == Number::COMPLEX  );
  }

  SECTION( "Overflows are detected" ) {
    Number maximum { std::numeric_limits<Number::smallint>::max() };
    REQUIRE( (maximum - small).hierarchyLevel() == Number::SMALLINT );
    REQUIRE( (maximum + small).hierarchyLevel() == Number::BIGINT   );
  }

}

TEST_CASE( "Number multiplication", "[number]" ) {

  Number    small { (Number::smallint)2 };
  Number      big { (Number::bigint)  2 };
  Number    ratio { Number::ratio(2, 1) };
  Number floating {                2.0  };
  Number  complex {          2.0 + 0.0i };

  SECTION( "Multiplication behaves correctly" ) {
    REQUIRE( small * small == 4l );
    REQUIRE( small *   big == 4l );
    REQUIRE( small * ratio == 4l );
    REQUIRE(   big *   big == 4l );
    REQUIRE(   big * ratio == 4l );
    REQUIRE( ratio * ratio == 4l );
  }

  SECTION( "Multiplication coerces to the larger type" ) {
    REQUIRE( (   small *    small).hierarchyLevel() == Number::SMALLINT );
    REQUIRE( (   small *      big).hierarchyLevel() == Number::BIGINT   );
    REQUIRE( (   small *    ratio).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   small * floating).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   small *  complex).hierarchyLevel() == Number::COMPLEX  );
  }

  SECTION( "Overflows are detected" ) {
    Number maximum { std::numeric_limits<Number::smallint>::max() };
    Number one { (Number::smallint)1 };
    REQUIRE( (maximum *   one).hierarchyLevel() == Number::SMALLINT );
    REQUIRE( (maximum * small).hierarchyLevel() == Number::BIGINT   );
  }

}

TEST_CASE( "Numerical division and reciprocation", "[number]" ) {

  Number    smallOne { (Number::smallint)1 };
  Number      bigOne { (Number::bigint)  1 };
  Number    ratioOne { Number::ratio(1, 1) };
  Number floatingOne {                1.0  };
  Number  complexOne {          1.0 + 0.0i };

  Number    smallTwo { (Number::smallint)2 };
  Number      bigTwo { (Number::bigint)  2 };
  Number    ratioTwo { Number::ratio(2, 1) };
  Number floatingTwo {                2.0  };
  Number  complexTwo {          2.0 + 0.0i };

  SECTION( "Division and reciprocation behave correctly" ) {
    REQUIRE( (smallOne / smallTwo) == Number::ratio(1, 2) );
    REQUIRE( (  bigOne / smallTwo) == Number::ratio(1, 2) );
    REQUIRE( (  bigOne /   bigTwo) == Number::ratio(1, 2) );
    REQUIRE( (smallOne /   bigTwo) == Number::ratio(1, 2) );
    REQUIRE( (  bigTwo / smallOne) == Number::ratio(2, 1) );
    REQUIRE( (ratioTwo / ratioOne) == Number::ratio(2, 1) );
  }

  SECTION( "Reciprocation widens appropriately" ) {
    REQUIRE(    smallOne.recip().hierarchyLevel() == Number::RATIO    );
    REQUIRE(      bigOne.recip().hierarchyLevel() == Number::RATIO    );
    REQUIRE(    ratioOne.recip().hierarchyLevel() == Number::RATIO    );
    REQUIRE( floatingOne.recip().hierarchyLevel() == Number::FLOATING );
    REQUIRE(  complexOne.recip().hierarchyLevel() == Number::COMPLEX  );
  }

  SECTION( "Division widens appropriately" ) {
    REQUIRE( (   smallOne /    smallOne).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   smallOne /      bigOne).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   smallOne /    ratioOne).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   smallOne / floatingOne).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   smallOne /  complexOne).hierarchyLevel() == Number::COMPLEX  );
  }

}

TEST_CASE( "Numerical modulo", "[number]" ) {

  Number    small { (Number::smallint)3 };
  Number      big { (Number::bigint)  3 };
  Number    ratio { Number::ratio(3, 1) };
  Number floating {                3.0  };
  Number  complex {          3.0 + 0.0i };

  SECTION( "Modulo behaves correctly" ) {
    REQUIRE( (small % 2l) == 1l );
    REQUIRE( (  big % 2l) == 1l );
    REQUIRE( (ratio % 2l) == 1l );
  }

  SECTION( "Modulo widens appropriately" ) {
    REQUIRE( (   small %    small).hierarchyLevel() == Number::SMALLINT );
    REQUIRE( (   small %      big).hierarchyLevel() == Number::BIGINT   );
    REQUIRE( (   small %    ratio).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   small % floating).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   small %  complex).hierarchyLevel() == Number::COMPLEX  );
  }

  SECTION( "Modulo behaves correctly given negative numbers" ) {
    REQUIRE( ( small %  2l) ==  1l );
    REQUIRE( ( small % -2l) == -1l );
    REQUIRE( (-small %  2l) ==  1l );
    REQUIRE( (-small % -2l) == -1l );
    REQUIRE( (   big %  2l) ==  1l );
    REQUIRE( (   big % -2l) == -1l );
    REQUIRE( (-  big %  2l) ==  1l );
    REQUIRE( (-  big % -2l) == -1l );
    REQUIRE( ( ratio %  2l) ==  1l );
    REQUIRE( ( ratio % -2l) == -1l );
    REQUIRE( (-ratio %  2l) ==  1l );
    REQUIRE( (-ratio % -2l) == -1l );
  }

  SECTION( "Modulo handles rational results" ) {
    Number a { Number::ratio(3, 2) };
    Number b { 2l };
    REQUIRE( b % a == Number::ratio(1, 2) );
  }

}


TEST_CASE( "Exponents", "[number]" ) {

  Number    small { (Number::smallint)3 };
  Number      big { (Number::bigint)  3 };
  Number    ratio { Number::ratio(3, 1) };
  Number floating {                3.0  };
  Number  complex {          3.0 + 0.0i };

  SECTION( "Exponents behave correctly" ) {
    REQUIRE( (small.pow(small)) == 27l );
    REQUIRE( (  big.pow(  big)) == 27l );
    REQUIRE( (ratio.pow(small)) == 27l );
  }

  SECTION( "Exponentiation respects the convoluted casting rules" ) {
    REQUIRE( (   small.pow(     small)).hierarchyLevel() == Number::BIGINT   );
    REQUIRE( (   small.pow(       big)).hierarchyLevel() == Number::BIGINT   );
    REQUIRE( (   small.pow(     ratio)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   small.pow(  floating)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   small.pow(   complex)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( (     big.pow(     small)).hierarchyLevel() == Number::BIGINT   );
    REQUIRE( (     big.pow(       big)).hierarchyLevel() == Number::BIGINT   );
    REQUIRE( (     big.pow(     ratio)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (     big.pow(  floating)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (     big.pow(   complex)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( (   ratio.pow(     small)).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   ratio.pow(       big)).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   ratio.pow(     ratio)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   ratio.pow(  floating)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   ratio.pow(   complex)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( (floating.pow(     small)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (floating.pow(       big)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (floating.pow(     ratio)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (floating.pow(  floating)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (floating.pow(   complex)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( ( complex.pow(     small)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( ( complex.pow(       big)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( ( complex.pow(     ratio)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( ( complex.pow(  floating)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( ( complex.pow(   complex)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( (   small.pow(-    small)).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   small.pow(-      big)).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   small.pow(-    ratio)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   small.pow(- floating)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (     big.pow(-    small)).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (     big.pow(-      big)).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (     big.pow(-    ratio)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (     big.pow(- floating)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   ratio.pow(-    small)).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   ratio.pow(-      big)).hierarchyLevel() == Number::RATIO    );
    REQUIRE( (   ratio.pow(-    ratio)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (   ratio.pow(- floating)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (floating.pow(-    small)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (floating.pow(-      big)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (floating.pow(-    ratio)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( (floating.pow(- floating)).hierarchyLevel() == Number::FLOATING );
    REQUIRE( ( complex.pow(-    small)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( ( complex.pow(-      big)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( ( complex.pow(-    ratio)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( ( complex.pow(- floating)).hierarchyLevel() == Number::COMPLEX  );
    REQUIRE( ( complex.pow(   complex)).hierarchyLevel() == Number::COMPLEX  );
  }

}

// We could test the bitwise operators and trig functions here (also,
// log). But what is there really to test? They literally just
// delegate down to the C++ system calls.

TEST_CASE( "Floor function", "[number]" ) {

  Number    small { (Number::smallint)3 };
  Number      big { (Number::bigint)  3 };
  Number    ratio { Number::ratio(7, 2) };
  Number floating {                3.5  };

  SECTION( "Floor behaves correctly" ) {
    REQUIRE(    small.floor() == 3l );
    REQUIRE(      big.floor() == 3l );
    REQUIRE(    ratio.floor() == 3l );
    REQUIRE( floating.floor() == 3l );
  }

  SECTION( "Floor can narrow the type" ) {
    REQUIRE(   small.floor().hierarchyLevel() == Number::SMALLINT );
    REQUIRE(     big.floor().hierarchyLevel() == Number::BIGINT   );
    REQUIRE(   ratio.floor().hierarchyLevel() == Number::BIGINT   );
    REQUIRE(floating.floor().hierarchyLevel() == Number::BIGINT   );
  }

}

TEST_CASE( "Parts of complex numbers", "[number]" ) {

  Number complex0 { 1 + 2i };
  Number complex1 = complexNumber(1l, 2l);

  REQUIRE( complex0 == complex1 );
  REQUIRE( complex0.realPart() == 1l );
  REQUIRE( complex0.imagPart() == 2l );
  REQUIRE( complex1.realPart() == 1l );
  REQUIRE( complex1.imagPart() == 2l );

  REQUIRE( complex0.realPart().hierarchyLevel() == Number::FLOATING );
  REQUIRE( complex0.imagPart().hierarchyLevel() == Number::FLOATING );
  REQUIRE( complex1.realPart().hierarchyLevel() == Number::FLOATING );
  REQUIRE( complex1.imagPart().hierarchyLevel() == Number::FLOATING );

}

// Constants like epsilon, infinity, NaN could be tested for certain
// properties?

TEST_CASE( "Parsing integers", "[number]" ) {

  REQUIRE( parseInteger("D300") == Number(300l) );
  REQUIRE( parseInteger("X12C") == Number(300l) );
  REQUIRE( parseInteger("B100101100") == Number(300l) );
  REQUIRE( parseInteger("O454") == Number(300l) );
  REQUIRE( parseInteger("B112") == boost::none );
  REQUIRE( parseInteger("D+300") == Number(300l) );
  REQUIRE( parseInteger("X+12C") == Number(300l) );
  REQUIRE( parseInteger("B+100101100") == Number(300l) );
  REQUIRE( parseInteger("O+454") == Number(300l) );
  REQUIRE( parseInteger("B+112") == boost::none );
  REQUIRE( parseInteger("D-300") == Number(-300l) );
  REQUIRE( parseInteger("X-12C") == Number(-300l) );
  REQUIRE( parseInteger("B-100101100") == Number(-300l) );
  REQUIRE( parseInteger("O-454") == Number(-300l) );
  REQUIRE( parseInteger("B-112") == boost::none );

}
