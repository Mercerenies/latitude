#ifndef PATHNAME_HPP
#define PATHNAME_HPP

#include <string>

/// \file
///
/// \brief Functions for manipulating and retrieving filenames.

/// Returns the full path and name of the Latitude executable.
///
/// Note: This function is only defined if the operating system is
/// recognized.
///
/// \return the path name
std::string getExecutablePathname();

/// Returns the current working directory, usually the same as the one
/// returned by the `pwd` command.
///
/// Note: This function is only defined if the operating system is
/// recognized.
///
/// \return the directory path
std::string getWorkingDirectory();

/// Returns the directory of the argument, which should be an absolute
/// path.
///
/// Note: This function is only defined if the operating system is
/// recognized.
///
/// \param path an absolute path
/// \return the directory
std::string stripFilename(std::string path);

/// Returns the filename of the argument, which should be an absolute
/// path.
///
/// Note: This function is only defined if the operating system is
/// recognized.
///
/// \param path an absolute path
/// \return the filename, without the directory
std::string stripDirname(std::string path);

#endif // PATHNAME_HPP
