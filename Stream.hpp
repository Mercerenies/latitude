#ifndef _STREAM_HPP_
#define _STREAM_HPP_

#include <iostream>
#include <sstream>
#include <memory>
#include <string>

class Stream;

using StreamPtr = std::shared_ptr<Stream>;

/*
 * A stream object. Stream objects can be input, output, or both (as defined
 * by the `hasIn` and `hasOut` methods). The behavior of `in` on a stream for
 * which `hasIn` returns false (likewise, the behavior of `out` on a stream for
 * which `hasOut` returns false) is undefined.
 */
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

/*
 * A stream that binds to `cout`.
 */
class CoutStream : public Stream {
public:
    virtual bool hasOut();
    virtual void out(char);
    virtual void writeLine(std::string);
};

/*
 * A stream that binds to `cerr`.
 */
class CerrStream : public Stream {
public:
    virtual bool hasOut();
    virtual void out(char);
    virtual void writeLine(std::string);
};

/*
 * A stream that binds to `cin`.
 */
class CinStream : public Stream {
public:
    virtual bool hasIn();
    virtual char in();
    virtual std::string readLine();
};

// Functions for creating stream objects bound to the three default streams.
// These are NOT accessors; they allocate a new object each time they are called.
StreamPtr outStream();
StreamPtr inStream();
StreamPtr errStream();

#endif // _STREAM_HPP_
