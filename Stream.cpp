#include <fstream>
#include "Stream.hpp"

using namespace std;

class InFileStream : public Stream {
private:
    ifstream stream;
public:
    InFileStream(string name, FileMode mode);
    ~InFileStream();
    virtual bool hasIn();
    virtual char in();
    virtual std::string readLine();
    virtual bool isEof();
};

class OutFileStream : public Stream {
private:
    ofstream stream;
public:
    OutFileStream(string name, FileMode mode);
    ~OutFileStream();
    virtual bool hasOut();
    virtual void out(char);
    virtual void writeLine(std::string);
    virtual bool isEof();
};

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

bool Stream::isEof() {
    return true;
}

void Stream::close() {}

bool CoutStream::hasOut() {
    return true;
}

void CoutStream::out(char ch) {
    cout << ch;
}

void CoutStream::writeLine(string str) {
    cout << str << endl;
}

bool CoutStream::isEof() {
    return cout.eof();
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

bool CerrStream::isEof() {
    return cerr.eof();
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

bool CinStream::isEof() {
    return cin.eof();
}

FileStream::FileStream(string name, FileAccess access, FileMode mode)
    : stream() {
    switch (access) {
    case FileAccess::READ:
        stream = unique_ptr<Stream>(new InFileStream(name, mode));
        break;
    case FileAccess::WRITE:
        stream = unique_ptr<Stream>(new OutFileStream(name, mode));
        break;
    }
}

bool FileStream::hasOut() {
    return stream->hasOut();
}

bool FileStream::hasIn() {
    return stream->hasIn();
}

void FileStream::out(char ch) {
    return stream->out(ch);
}

char FileStream::in() {
    return stream->in();
}

void FileStream::writeLine(string line) {
    return stream->writeLine(line);
}

string FileStream::readLine() {
    return stream->readLine();
}

bool FileStream::isEof() {
    return stream->isEof();
}

void FileStream::close() {
    stream = unique_ptr<Stream>(new NullStream());
}

InFileStream::InFileStream(string name, FileMode mode)
    : stream(name, translateMode(mode)) {
    stream.exceptions( ios_base::failbit | ios_base::badbit );
}

InFileStream::~InFileStream() {
    stream.close();
}

bool InFileStream::hasIn() {
    return true;
}

char InFileStream::in() {
    return stream.get();
}

string InFileStream::readLine() {
    string result;
    getline(stream, result);
    return result;
}

bool InFileStream::isEof() {
    return stream.eof();
}

OutFileStream::OutFileStream(string name, FileMode mode)
    : stream(name, translateMode(mode)) {
    stream.exceptions( ios_base::failbit | ios_base::badbit );
}

OutFileStream::~OutFileStream() {
    stream.close();
}

bool OutFileStream::hasOut() {
    return true;
}

void OutFileStream::out(char ch) {
    stream << ch;
}

void OutFileStream::writeLine(string str) {
    stream << str << endl;
}

bool OutFileStream::isEof() {
    return stream.eof();
}

ios_base::openmode translateMode(FileMode fmode) {
    switch (fmode) {
    case FileMode::TEXT:
        return {};
    case FileMode::BINARY:
        return fstream::binary;
    }
    return {}; // -Wreturn-type asked nicely
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
