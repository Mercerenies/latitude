
#include "Precedence.hpp"
#include "Parents.hpp"

OperatorTable::OperatorTable(ObjectPtr table)
    : impl(table) {}

int OperatorTable::precedence(std::string op) const {
    Symbolic name = Symbols::get()[op];
    ObjectPtr result = objectGet(impl, name);
    // Doesn't exist? Fine, just use the default.
    if (result == nullptr)
        return DEFAULT_PRECEDENCE;
    // We have the object; it had better be a small number in the range now.
    if (auto value = boost::get<Number>(&result->prim())) {
        if (value->hierarchyLevel() > 0)
            throw ("Invalid operator table at " + op);
        auto value1 = value->asSmallInt();
        if ((value1 < 0) || (value1 >= 256))
            throw ("Invalid operator table at " + op);
        return (int)value1;
    } else {
        // Not a number
        throw ("Invalid operator table at " + op);
    }
}
