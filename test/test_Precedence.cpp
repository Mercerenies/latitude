//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "test.hpp"
#include "Precedence.hpp"
#include "Garnish.hpp"

TEST_CASE( "Operator table", "" ) {

  // The operator table doesn't actually exist, per se, until the core
  // library is loaded. So we'll mock it here.

  ObjectPtr eq    = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr plus  = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr minus = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr times = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr pow   = clone(globalVM->reader.lit[Lit::OBJECT]);

  ObjectPtr lex = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr meta = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr obj = clone(globalVM->reader.lit[Lit::OBJECT]);
  ObjectPtr impl = clone(globalVM->reader.lit[Lit::OBJECT]);

  eq->put(Symbols::get()["prec"], garnishObject(globalVM->reader, 5l));
  eq->put(Symbols::get()["assoc"], eq);
  eq->put(Symbols::get()["inner"], garnishObject(globalVM->reader, Symbols::get()["none"]));

  plus->put(Symbols::get()["prec"], garnishObject(globalVM->reader, 10l));
  plus->put(Symbols::get()["assoc"], plus);
  plus->put(Symbols::get()["inner"], garnishObject(globalVM->reader, Symbols::get()["left"]));

  minus->put(Symbols::get()["prec"], garnishObject(globalVM->reader, 10l));
  minus->put(Symbols::get()["assoc"], minus);
  minus->put(Symbols::get()["inner"], garnishObject(globalVM->reader, Symbols::get()["left"]));

  times->put(Symbols::get()["prec"], garnishObject(globalVM->reader, 20l));
  times->put(Symbols::get()["assoc"], times);
  times->put(Symbols::get()["inner"], garnishObject(globalVM->reader, Symbols::get()["left"]));

  pow->put(Symbols::get()["prec"], garnishObject(globalVM->reader, 30l));
  pow->put(Symbols::get()["assoc"], pow);
  pow->put(Symbols::get()["inner"], garnishObject(globalVM->reader, Symbols::get()["right"]));

  lex->put(Symbols::get()["meta"], meta);
  meta->put(Symbols::get()["operators"], obj);
  obj->put(Symbols::get()["&impl"], impl);

  impl->put(Symbols::get()["=="], eq);
  impl->put(Symbols::get()["+"], plus);
  impl->put(Symbols::get()["-"], minus);
  impl->put(Symbols::get()["*"], times);
  impl->put(Symbols::get()["^"], pow);

  SECTION( "Getting the precedence table" ) {

    OperatorTable table = getTable(lex);

    auto eqData = table.lookup("==");
    REQUIRE( eqData.precedence == 5 );
    REQUIRE( eqData.associativity == Associativity::NONE );

    auto minusData = table.lookup("-");
    REQUIRE( minusData.precedence == 10 );
    REQUIRE( minusData.associativity == Associativity::LEFT );

    auto nonsenseData = table.lookup("&**=+");
    REQUIRE( nonsenseData.precedence == DEFAULT_PRECEDENCE );
    REQUIRE( nonsenseData.associativity == Associativity::LEFT );

  }

  SECTION( "Testing precedence" ) {

    // Original:
    //   1 + 2 - 3 * 4 == 5.
    // Should result in:
    //   ((1 + 2) - (3 * 4)) == 5.
    Expr* expr = new Expr();

    // ---

    Expr* curr = expr;

    curr->lhs = new Expr();

    curr->isOperator = true;
    curr->name = new char[3]{'=', '=', '\0'};

    curr->args = new List();
    curr->args->car = new Expr();
    curr->args->car->line = 5;
    curr->args->cdr = new List();

    // ---

    curr = curr->lhs;

    curr->lhs = new Expr();

    curr->isOperator = true;
    curr->name = new char[2]{'*', '\0'};

    curr->args = new List();
    curr->args->car = new Expr();
    curr->args->car->line = 4;
    curr->args->cdr = new List();

    // ---

    curr = curr->lhs;

    curr->lhs = new Expr();

    curr->isOperator = true;
    curr->name = new char[2]{'-', '\0'};

    curr->args = new List();
    curr->args->car = new Expr();
    curr->args->car->line = 3;
    curr->args->cdr = new List();

    // ---

    curr = curr->lhs;

    curr->lhs = new Expr();

    curr->isOperator = true;
    curr->name = new char[2]{'+', '\0'};

    curr->args = new List();
    curr->args->car = new Expr();
    curr->args->car->line = 2;
    curr->args->cdr = new List();

    // ---

    curr = curr->lhs;
    curr->line = 1;

    // --- ---

    // Leaking memory, I think. The precedence code does some weird pointer stuff >.>
    Expr* result = reorganizePrecedence(OperatorTable(impl), expr);

    //   ((1 + 2) - (3 * 4)) == 5.
    REQUIRE( strcmp(result->name, "==") == 0 );
    REQUIRE( strcmp(result->lhs->name, "-") == 0 );
    REQUIRE( strcmp(result->lhs->lhs->name, "+") == 0 );
    REQUIRE( strcmp(result->lhs->args->car->name, "*") == 0 );

    REQUIRE( result->lhs->lhs->lhs->line == 1 );
    REQUIRE( result->lhs->lhs->args->car->line == 2 );
    REQUIRE( result->lhs->args->car->lhs->line == 3 );
    REQUIRE( result->lhs->args->car->args->car->line == 4 );
    REQUIRE( result->args->car->line == 5 );

    cleanupE(result);

  }

}
