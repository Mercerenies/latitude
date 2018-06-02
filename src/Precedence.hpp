#ifndef PRECEDENCE_HPP
#define PRECEDENCE_HPP

#include "Proto.hpp"
#include "Parser.tab.h"

constexpr int DEFAULT_PRECEDENCE = 0;

constexpr int MIN_PRECEDENCE = 0;
constexpr int MAX_PRECEDENCE = 255;

enum class Associativity {
    LEFT,
    RIGHT,
    NONE
};

struct OperatorData {
    int precedence;
    Associativity associativity;
};

class OperatorTable {
private:
    ObjectPtr impl;
public:
    explicit OperatorTable(ObjectPtr table);
    OperatorData lookup(std::string op) const;
};

Expr* reorganizePrecedence(const OperatorTable& table, Expr* expr);

OperatorTable getTable(ObjectPtr lex);

#endif // PRECEDENCE_HPP
