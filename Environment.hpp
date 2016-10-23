#ifndef _ENVIRONMENT_HPP_
#define _ENVIRONMENT_HPP_

#include <string>
#include <boost/optional.hpp>

boost::optional<std::string> getEnv(std::string name);

bool setEnv(std::string name, std::string value); // Returns whether supported

bool unsetEnv(std::string name); // Returns whether supported

#endif
