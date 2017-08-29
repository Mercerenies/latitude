#ifndef PATHNAME_HPP
#define PATHNAME_HPP

#include <string>

std::string getExecutablePathname();

std::string stripFilename(std::string path);

std::string stripDirname(std::string path);

#endif // PATHNAME_HPP
