//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef TEST_HPP
#define TEST_HPP

#include <sstream>
#include "Bytecode.hpp"
#include "Dump.hpp"
#include "catch2/catch.hpp"

extern VMState* globalVM;

namespace Catch {

  template <>
  struct StringMaker<ObjectPtr> {
    static std::string convert(const ObjectPtr& value) {
      std::ostringstream oss;
      oss << DebugObject{ value };
      return oss.str();
    }
  };

}

#endif // TEST_HPP
