#include "Stream.hpp"

using namespace std;

char Stream::in() {
    return 0;
}

void Stream::out(char ch) {}

bool Stream::hasIn() {
    return false;
}

bool Stream::hasOut() {
    return false;
}

string Stream::readLine() {
    ostringstream out;
    char ch = in();
    while ((ch != 13) && (ch != 10)) {
        out << ch;
        ch = in();
    }
    return out.str();
}

string Stream::readText(int n) {
    ostringstream out;
    for (int i = 0; i < n; i++) {
        char ch = in();
        out << ch;
    }
    return out.str();
}

void Stream::writeLine(string str) {
    writeText(str);
    out('\n');
}

void Stream::writeText(string str) {
    for (char ch : str)
        out(ch);
}

bool CoutStream::hasOut() {
    return true;
}

void CoutStream::out(char ch) {
    cout << ch;
}

void CoutStream::writeLine(string str) {
    cout << str << endl;
}

bool CerrStream::hasOut() {
    return true;
}

void CerrStream::out(char ch) {
    cerr << ch;
}

void CerrStream::writeLine(string str) {
    cerr << str << endl;
}

bool CinStream::hasIn() {
    return true;
}

char CinStream::in() {
    return cin.get();
}

string CinStream::readLine() {
    string result;
    getline(cin, result);
    return result;
}

StreamPtr outStream() {
    return StreamPtr(new CoutStream());
}

StreamPtr inStream() {
    return StreamPtr(new CinStream());
}

StreamPtr errStream() {
    return StreamPtr(new CerrStream());
}
