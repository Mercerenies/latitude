#ifndef _STREAM_HPP_
#define _STREAM_HPP_

#include <iostream>
#include <sstream>
#include <memory>
#include <string>

class Stream;

using StreamPtr = std::shared_ptr<Stream>;

class Stream {
public:
    virtual ~Stream() = default;
    virtual char in();
    virtual void out(char);
    virtual bool hasIn();
    virtual bool hasOut();
    virtual std::string readLine();
    virtual std::string readText(int);
    virtual void writeLine(std::string);
    virtual void writeText(std::string);
};

class CoutStream : public Stream {
public:
    virtual bool hasOut();
    virtual void out(char);
    virtual void writeLine(std::string);
};

class CerrStream : public Stream {
public:
    virtual bool hasOut();
    virtual void out(char);
    virtual void writeLine(std::string);
};

class CinStream : public Stream {
public:
    virtual bool hasIn();
    virtual char in();
    virtual std::string readLine();
};

StreamPtr outStream();
StreamPtr inStream();
StreamPtr errStream();

#endif // _STREAM_HPP_
