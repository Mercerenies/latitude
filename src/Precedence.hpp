//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef PRECEDENCE_HPP
#define PRECEDENCE_HPP

#include "Proto.hpp"
#include "Parser.tab.h"

/// \file
/// \brief Classes and functions for resolving Latitude operator precedence

/// \brief The default precedence for operators.
///
/// The default precedence for operators which do not explicitly
/// appear in the operator table.
constexpr int DEFAULT_PRECEDENCE = 30;

/// The minimum valid value for operator precedence.
constexpr int MIN_PRECEDENCE = 0;
/// The maximum valid value for operator precedence.
constexpr int MAX_PRECEDENCE = 255;

/// Specifies the associativity of an operator.
enum class Associativity {
    /// Left associativity.
    LEFT,
    /// Right associativity.
    RIGHT,
    /// No associativity.
    NONE
};

/// This is the type of values returned from OperatorTable#lookup,
/// specifying the precedence and associativity of an operator.
struct OperatorData {
    /// The precedence, an integer from 0 to 255 (inclusive).
    int precedence;
    /// The associativity.
    Associativity associativity;
};

/// The operator table itself. OperatorTable instances perform lookups
/// in the implementing table object, which is passed to the
/// constructor.
class OperatorTable {
private:
    ObjectPtr impl;
public:

    /// Constructs an OperatorTable from a non-null object pointer.
    /// Note that the argument is *not* the dictionary object but its
    /// internal implementation object, which should be a "fresh"
    /// object.
    ///
    /// \param table the table object
    explicit OperatorTable(ObjectPtr table);

    /// Looks up the operator in the table. If it exists, the
    /// appropriate data is returned. If it does not, then a default
    /// OperatorData object consisting of ::DEFAULT_PRECEDENCE and
    /// Associativity::LEFT is returned.
    ///
    /// \param op the operator name
    /// \return the data associated with the operator
    OperatorData lookup(std::string op) const;

};

/// Uses the operator table to reorganize the expression with regard
/// to precedence. The returned expression pointer may or may not be
/// related to the argument pointer. In general, the argument pointer
/// will be left in a valid but indetermined state for cleanup
/// purposes.
///
/// \param table the operator table
/// \param expr the expression to parse
/// \return the new expression
Expr* reorganizePrecedence(const OperatorTable& table, Expr* expr);

/// Looks up the operator table based on the current lexical scope.
///
/// \param lex a lexical scoping object
/// \return the operator table
OperatorTable getTable(ObjectPtr lex);

#endif // PRECEDENCE_HPP
