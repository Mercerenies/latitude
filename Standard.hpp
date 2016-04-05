#ifndef _STANDARD_HPP_
#define _STANDARD_HPP_

#include <string>
#include "Proto.hpp"
#include "Cont.hpp"

ObjectPtr spawnObjects();
ProtoError doSlotError(ObjectPtr& global, ObjectPtr& problem, std::string slotName);
ProtoError doParseError(ObjectPtr& global);
ProtoError doParseError(ObjectPtr& global, std::string message);

#endif // _STANDARD_HPP_
