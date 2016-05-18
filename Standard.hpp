#ifndef _STANDARD_HPP_
#define _STANDARD_HPP_

#include <string>
#include "Proto.hpp"
#include "Bytecode.hpp"

ObjectPtr spawnObjects(IntState& state);

void throwError(IntState& state, std::string name, std::string msg);
void throwError(IntState& state, std::string name);

#endif // _STANDARD_HPP_
