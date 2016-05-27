#include "Header.hpp"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>

using namespace std;

Header getFileHeader(std::string filename) {
    Header header;
    ifstream file(filename);
    if (!file)
        return header;
    auto is_blank_line = [](string line) {
        return all_of<string::iterator, int(*)(int)>(line.begin(), line.end(), isspace);
    };
    // Skip blank lines at the start of the file (if any)
    std::string curr;
    getline(file, curr);
    while (!file.eof() && is_blank_line(curr))
        getline(file, curr);
    // Start reading header lines
    while (!file.eof() && curr.substr(0, 3) == ";;*") {
        istringstream iss(curr);
        string ignore, keyword, rest;
        iss >> ignore >> keyword >> rest;
        if (keyword == "MODULE") {
            header.module = rest;
            header.fields |= (unsigned int)HeaderField::MODULE;
        } else if (keyword == "PACKAGE") {
            header.package = rest;
            header.fields |= (unsigned int)HeaderField::PACKAGE;
        }
        getline(file, curr);
    }
    file.close();
    return header;
}
