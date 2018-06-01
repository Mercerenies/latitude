#ifndef PRECEDENCE_HPP
#define PRECEDENCE_HPP

#include "Proto.hpp"

constexpr int DEFAULT_PRECEDENCE = 0;

class OperatorTable {
private:
    ObjectPtr impl;
public:
    explicit OperatorTable(ObjectPtr table);
    int precedence(std::string op) const;
};

#endif // PRECEDENCE_HPP
