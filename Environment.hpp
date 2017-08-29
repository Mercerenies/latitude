#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <string>
#include <boost/optional.hpp>

boost::optional<std::string> getEnv(std::string name);

bool setEnv(std::string name, std::string value); // Returns whether supported

bool unsetEnv(std::string name); // Returns whether supported

#endif // ENVIRONMENT_HPP
