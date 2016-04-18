#include "Number.hpp"
#include <limits>

using namespace std;

Number::Number(smallint arg)
    : value(arg) {}

Number::Number(bigint arg)
    : value(arg) {}

Number::Number(ratio arg)
    : value(arg) {}

Number::Number(floating arg)
    : value(arg) {}

bool Number::operator ==(const Number& other) {
    auto second = other.value;
    return boost::apply_visitor(MagicNumber::EqualVisitor(), value, second);
}

bool Number::operator <(const Number& other) {
    auto second = other.value;
    return boost::apply_visitor(MagicNumber::LessVisitor(), value, second);
}

Number& Number::operator +=(const Number& other) {
    auto second = other.value;
    if ((value.which() == 0) && (second.which() == 0)) {
        // Possibility of overflow (two smallints)
        bool promote = false;
        smallint x0 = boost::get<smallint>(value);
        smallint x1 = boost::get<smallint>(second);
        if ((x1 > 0) && (x0 > numeric_limits<smallint>::max() - x1))
            promote = true;
        if ((x1 < 0) && (x0 < numeric_limits<smallint>::min() - x1))
            promote = true;
        if (promote)
            value = bigint(x0);
    }
    value = boost::apply_visitor(MagicNumber::PlusVisitor(), value, second);
    return *this;
}

Number& Number::operator -=(const Number& other) {
    return (*this += -other);
}

Number& Number::operator *=(const Number& other) {
    auto second = other.value;
    if ((value.which() == 0) && (second.which() == 0)) {
        // Possibility of overflow (two smallints)
        bool promote = false;
        smallint x0 = boost::get<smallint>(value);
        smallint x1 = boost::get<smallint>(second);
        if ((x1 != 0) && (abs(x0) > numeric_limits<smallint>::max() / abs(x1)))
            promote = true;
        if (promote)
            value = bigint(x0);
    }
    value = boost::apply_visitor(MagicNumber::TimesVisitor(), value, second);
    return *this;
}

Number& Number::operator /=(const Number& other) {
    return (*this *= other.recip());
}

Number& Number::operator %=(const Number& other) {
    auto second = other.value;
    value = boost::apply_visitor(MagicNumber::ModVisitor(), value, second);
    return *this;
}

Number Number::operator -() const {
    Number curr = *this;
    curr.value = boost::apply_visitor(MagicNumber::NegateVisitor(), curr.value);
    return curr;
}

Number Number::recip() const {
    Number curr = *this;
    curr.value = boost::apply_visitor(MagicNumber::RecipVisitor(), curr.value);
    return curr;
}

string Number::asString() const {
    MagicNumber::StringifyVisitor visitor;
    boost::apply_visitor(visitor, value);
    return visitor.str();
}

int Number::hierarchyLevel() const {
    return boost::apply_visitor(MagicNumber::LevelVisitor(), value);
}

namespace MagicNumber {
    StringifyVisitor::StringifyVisitor()
        : stream() {}
    void StringifyVisitor::operator()(const double& first) {
        stream << fixed << setprecision(2) << first;
    }
    std::string StringifyVisitor::str() {
        return stream.str();
    }
}
