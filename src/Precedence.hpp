#ifndef PRECEDENCE_HPP
#define PRECEDENCE_HPP

#include "Proto.hpp"

constexpr int DEFAULT_PRECEDENCE = 0;

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

#endif // PRECEDENCE_HPP
