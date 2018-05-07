#ifndef DUMP_HPP
#define DUMP_HPP

#include <ostream>
#include "Proto.hpp"

/// An adaptor class which provides useful dumping and printing
/// capabilities for object pointers.
class DebugObject {
private:
    ObjectPtr impl;
public:
    DebugObject(ObjectPtr ptr);
    ObjectPtr get() const;
    friend std::ostream& operator <<(std::ostream& out, const DebugObject& obj);
};

std::ostream& operator <<(std::ostream& out, const DebugObject& obj);

#endif // DUMP_HPP
