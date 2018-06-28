//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef STREAM_HPP
#define STREAM_HPP

#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <ios>

class Stream;

using StreamPtr = std::shared_ptr<Stream>;

enum class FileAccess { READ, WRITE };
enum class FileMode { TEXT, BINARY };

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
    virtual bool hasIn() const noexcept;
    virtual bool hasOut() const noexcept;
    virtual std::string readLine();
    virtual std::string readText(int);
    virtual void writeLine(std::string);
    virtual void writeText(std::string);
    virtual bool isEof() const noexcept;
    virtual void close();
    virtual void flush();
};

/*
 * An empty stream that has neither read nor write priveleges. This
 * is used to represent a file that has already been closed.
 */
class NullStream : public Stream {};

/*
 * A stream that binds to `cout`.
 */
class CoutStream : public Stream {
public:
    virtual bool hasOut() const noexcept;
    virtual void out(char);
    virtual void writeLine(std::string);
    virtual bool isEof() const noexcept;
    virtual void flush();
};

/*
 * A stream that binds to `cerr`.
 */
class CerrStream : public Stream {
public:
    virtual bool hasOut() const noexcept;
    virtual void out(char);
    virtual void writeLine(std::string);
    virtual bool isEof() const noexcept;
    virtual void flush();
};

/*
 * A stream that binds to `cin`.
 */
class CinStream : public Stream {
public:
    virtual bool hasIn() const noexcept;
    virtual char in();
    virtual std::string readLine();
    virtual bool isEof() const noexcept;
};

/*
 * A stream that binds to an output file.
 */
class FileStream : public Stream {
private:
    std::unique_ptr<Stream> stream;
public:
    FileStream(std::string name, FileAccess access, FileMode mode);
    virtual ~FileStream() = default;
    virtual bool hasOut() const noexcept;
    virtual bool hasIn() const noexcept;
    virtual void out(char);
    virtual char in();
    virtual void writeLine(std::string);
    virtual std::string readLine();
    virtual bool isEof() const noexcept;
    virtual void close();
    virtual void flush();
};

std::ios_base::openmode translateMode(FileMode);

// Functions for creating stream objects bound to the three default streams.
// These are NOT accessors; they allocate a new object each time they are called.
StreamPtr outStream();
StreamPtr inStream();
StreamPtr errStream();

#endif // STREAM_HPP
