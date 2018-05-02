#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <string>
#include <optional>

/// \file
///
/// \brief Functionality for accessing system environment variables

/// Retrieves the value of the environment variable with the given name.
///
/// \param name the name of the environment variable
/// \return the value of the variable, or an empty value if the variable doesn't exist
std::optional<std::string> getEnv(std::string name);

/// Sets the value of the given environment variable. If the variable
/// does not exist, it is created. This function may not be supported
/// on unrecognized platforms.
///
/// \param name the name of the environment variable
/// \param value the value to assign to the variable
/// \return whether or not the operation was supported
bool setEnv(std::string name, std::string value);

/// Unsets the given environment variable. If the variable does not
/// exist then this function is a no-op. This function may not be
/// supported on unrecognized platforms.
///
/// \param name the name of the environment variable
/// \return whether or not the operation was supported
bool unsetEnv(std::string name);

#endif // ENVIRONMENT_HPP
