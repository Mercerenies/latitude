#ifndef STANDARD_HPP
#define STANDARD_HPP

#include <string>
#include "Proto.hpp"
#include "Bytecode.hpp"

ObjectPtr spawnObjects(IntState& state, ReadOnlyState& reader);

void throwError(IntState& state, std::string name, std::string msg);
void throwError(IntState& state, std::string name);

#endif // STANDARD_HPP
