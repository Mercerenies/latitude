
#include <utility>
#include <list>
#include "Precedence.hpp"
#include "Parents.hpp"
#include "Reader.hpp"

// An empty string signals the end of input
using op_pair_t = std::pair<List*, std::string>;

OperatorTable::OperatorTable(ObjectPtr table)
    : impl(table) {}

OperatorData OperatorTable::lookup(std::string op) const {
    Symbolic name = Symbols::get()[op];
    ObjectPtr result = objectGet(impl, name);
    // Doesn't exist? Fine, just use the default.
    if (result == nullptr)
        return { DEFAULT_PRECEDENCE, Associativity::LEFT };
    // We have the object; it had better be a small number in the range now.
    if (auto value = boost::get<Number>(&result->prim())) {
        if (value->hierarchyLevel() > 0)
            throw ("Invalid operator table at " + op);
        auto value1 = value->asSmallInt();
        if ((value1 < 0) || (value1 >= 256))
            throw ("Invalid operator table at " + op);
        OperatorData result;
        result.precedence = (int)value1;
        // Note: Everything is left associative at the moment. That
        // may (will?) change in the future.
        result.associativity = Associativity::LEFT;
        return result;
    } else {
        // Not a number
        throw ("Invalid operator table at " + op);
    }
}

std::list<op_pair_t> exprToSeq(Expr* expr) {
    std::list<op_pair_t> result;
    std::string op = "";
    while ((expr != nullptr) && (expr->isOperator)) {
        result.push_front({ expr->args, op });
        op = std::string(expr->name);
        Expr* temp = expr;
        expr = expr->lhs;
        temp->lhs = nullptr;
        temp->args = nullptr;
        cleanupE(temp);
    }
    List* dummy = makeList();
    dummy->car = expr;
    dummy->cdr = nullptr;
    result.push_front({ dummy, op });
    return result;
}
