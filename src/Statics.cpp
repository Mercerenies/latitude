//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

// I need to guarantee the destruction order of these two singletons,
// so I'm placing them in a separate translation unit together.

#include "GC.hpp"
#include "Allocator.hpp"

GC GC::instance = GC();
Allocator Allocator::instance = Allocator();
