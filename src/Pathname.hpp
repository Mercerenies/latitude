//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef PATHNAME_HPP
#define PATHNAME_HPP

#include <string>
#include <ctime>

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

/// Returns the modification date of the argument, which should be a
/// path.
///
/// Note: This function is only defined if the operating system is
/// recognized.
///
/// \param path a path name
/// \return the modification time
std::time_t modificationTime(std::string path);

/// Returns whether a file with the given name exists.
///
/// \param fname the path name, either relative or absolute
/// \return whether the file exists
bool fileExists(std::string fname);

#endif // PATHNAME_HPP
