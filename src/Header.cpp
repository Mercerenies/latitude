//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details


#include "Header.hpp"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>

using namespace std;

Header getFileHeaderSource(std::ifstream& file) {
    Header header;
    header.fields = 0;
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
    return header;
}

// Throws HeaderError
Header getFileHeaderComp(std::ifstream& file) {
    std::istream_iterator<unsigned char> iter { file };
    Header value = deserialize<Header>(iter);
    // file.ignore(1); // Don't do this; the iterator already kinda sorta did this
    return value;
}

void saveFileHeader(std::ofstream& file, const Header& header) {
    std::vector<unsigned char> bytestream;
    bytestream.reserve(256);
    std::ostream_iterator<unsigned char> iter { file };
    serialize(header, iter);
    file << '.';
}
