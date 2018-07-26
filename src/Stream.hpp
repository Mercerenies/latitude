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

/// A shared pointer to a Stream object.
using StreamPtr = std::shared_ptr<Stream>;

/// A file access mode. Specifies whether the file is for reading or
/// writing.
enum class FileAccess { READ, WRITE };

/// A file data mode. Specifies whether the file stores text or binary
/// data.
enum class FileMode { TEXT, BINARY };

/// A stream object. Stream objects can be input, output, or both (as
/// defined by the Stream::hasIn and Stream::hasOut methods).
class Stream {
public:

    virtual ~Stream() = default;

    /// Reads in a single character from the stream. By default,
    /// Stream objects are neither readable nor writable, so this
    /// method should be overridden in read streams.
    ///
    /// \return the character
    virtual char in();

    /// Writes a single character to the stream. By default, Stream
    /// objects are neither readable nor writable, so this method
    /// should be overridden in write streams.
    ///
    /// \param ch the character to write
    virtual void out(char ch);

    /// \return whether the stream is designated for input.
    virtual bool hasIn() const noexcept;

    /// \return whether the stream is designated for output.
    virtual bool hasOut() const noexcept;

    /// Reads a full line of input from the stream. The stream must
    /// have been designated for input. A line is terminated by '\\r',
    /// '\\n', or EOF.
    ///
    /// \return the string, excluding the newline character
    virtual std::string readLine();

    /// Reads N characters from a stream which has at least N
    /// characters remaining. The stream must have been designated for
    /// input.
    ///
    /// \param n the number of characters, nonnegative
    /// \return the data
    virtual std::string readText(int n);

    /// Writes a line of text to the stream, followed by '\\n'. The
    /// stream must have been designated for output.
    ///
    /// \param str the string
    virtual void writeLine(std::string str);

    /// Writes a line of text to the stream. The stream must have been
    /// designated for output.
    ///
    /// \param str the string
    virtual void writeText(std::string str);

    /// Returns whether the stream is at its end. Any nontrivial
    /// subclass of Stream should override this method, as the default
    /// implementation simply always returns true.
    ///
    /// \return whether the stream is at EOF
    virtual bool isEof() const noexcept;

    /// Closes the stream.
    virtual void close();

    /// Flushes any buffered output to the stream, which must be
    /// designated for output.
    virtual void flush();
};

/// An empty stream that has neither read nor write priveleges. This
/// is used to represent a file that has already been closed.
class NullStream : public Stream {};

/// A stream that binds to `cout`. Output only.
class CoutStream : public Stream {
public:
    virtual bool hasOut() const noexcept;
    virtual void out(char);
    virtual void writeLine(std::string);
    virtual bool isEof() const noexcept;
    virtual void flush();
};

/// A stream that binds to `cerr`. Output only.
class CerrStream : public Stream {
public:
    virtual bool hasOut() const noexcept;
    virtual void out(char);
    virtual void writeLine(std::string);
    virtual bool isEof() const noexcept;
    virtual void flush();
};

/// A stream that binds to `cin`. Input only.
class CinStream : public Stream {
public:
    virtual bool hasIn() const noexcept;
    virtual char in();
    virtual std::string readLine();
    virtual bool isEof() const noexcept;
};

/// A stream that binds to a file. The input/output mode of the stream
/// depends on how the file is opened.
class FileStream : public Stream {
private:
    std::unique_ptr<Stream> stream;
public:
    /// Constructs a FileStream.
    ///
    /// \param name the file name
    /// \param access whether the stream is for input or output
    /// \param mode whether the file contains text or binary data
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

/// Converts a FileMode value to the appropriate ios_base bitflag
/// value. The returned value is appropriate for bitwise-or
/// application to any other desired flags.
///
/// \param fmode the file mode
/// \return the openmode value
std::ios_base::openmode translateMode(FileMode fmode);

// Functions for creating stream objects bound to the three default
// streams. These are NOT accessors; they allocate a new object each
// time they are called.

/// \return a pointer to a cout stream.
StreamPtr outStream();

/// \return a pointer to a cin stream.
StreamPtr inStream();

/// \return a pointer to a cerr stream.
StreamPtr errStream();

#endif // STREAM_HPP
