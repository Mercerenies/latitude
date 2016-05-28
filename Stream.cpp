#include <fstream>
#include "Stream.hpp"

using namespace std;

// TODO Binary file prints a \377 at the end for some odd reason

class InFileStream : public Stream {
private:
    ifstream stream;
public:
    InFileStream(string name, FileMode mode);
    ~InFileStream();
    virtual bool hasIn() const noexcept;
    virtual char in();
    virtual std::string readLine();
    virtual bool isEof() const noexcept;
};

class OutFileStream : public Stream {
private:
    ofstream stream;
public:
    OutFileStream(string name, FileMode mode);
    ~OutFileStream();
    virtual bool hasOut() const noexcept;
    virtual void out(char);
    virtual void writeLine(std::string);
    virtual bool isEof() const noexcept;
};

char Stream::in() {
    return 0;
}

void Stream::out(char ch) {}

bool Stream::hasIn() const noexcept {
    return false;
}

bool Stream::hasOut() const noexcept {
    return false;
}

string Stream::readLine() {
    ostringstream out;
    char ch = in();
    while ((ch != 13) && (ch != 10) && (!isEof())) {
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

bool Stream::isEof() const noexcept {
    return true;
}

void Stream::close() {}

bool CoutStream::hasOut() const noexcept {
    return true;
}

void CoutStream::out(char ch) {
    cout << ch;
}

void CoutStream::writeLine(string str) {
    cout << str << endl;
}

bool CoutStream::isEof() const noexcept {
    return cout.eof();
}

bool CerrStream::hasOut() const noexcept {
    return true;
}

void CerrStream::out(char ch) {
    cerr << ch;
}

void CerrStream::writeLine(string str) {
    cerr << str << endl;
}

bool CerrStream::isEof() const noexcept {
    return cerr.eof();
}

bool CinStream::hasIn() const noexcept {
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

bool CinStream::isEof() const noexcept {
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

bool FileStream::hasOut() const noexcept {
    return stream->hasOut();
}

bool FileStream::hasIn() const noexcept {
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

bool FileStream::isEof() const noexcept {
    return stream->isEof();
}

void FileStream::close() {
    stream = unique_ptr<Stream>(new NullStream());
}

InFileStream::InFileStream(string name, FileMode mode)
    : stream(name, translateMode(mode)) {
    stream.exceptions( ios_base::badbit );
}

InFileStream::~InFileStream() {
    stream.close();
}

bool InFileStream::hasIn() const noexcept {
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

bool InFileStream::isEof() const noexcept {
    return stream.eof();
}

OutFileStream::OutFileStream(string name, FileMode mode)
    : stream(name, translateMode(mode)) {
    stream.exceptions( ios_base::badbit );
}

OutFileStream::~OutFileStream() {
    stream.close();
}

bool OutFileStream::hasOut() const noexcept {
    return true;
}

void OutFileStream::out(char ch) {
    stream << ch;
}

void OutFileStream::writeLine(string str) {
    stream << str << endl;
}

bool OutFileStream::isEof() const noexcept {
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
