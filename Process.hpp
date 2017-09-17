#ifndef PROCESS_HPP
#define PROCESS_HPP

#include "Stream.hpp"
#include <string>

/// \file
///
/// \brief The Process class and a platform-specific constructor.

/// A Process instance represents an external instance of some program
/// that is running as a separate process. The Process class itself is
/// abstract and should be instantiated using makeProcess(), which
/// chooses the correct implementation based on the operating system.
class Process {
protected:
    std::string cmd;
    StreamPtr in;
    StreamPtr out;
    StreamPtr err;

    /// The internal implementation that actually runs the process. It
    /// should be called at most once for each Process instance.
    ///
    /// \return zero if successful, nonzero otherwise
    virtual int _run() = 0;

public:

    /// Constructs a process with the given name.
    Process(std::string cmd);

    /// Destructs the Process instance.
    virtual ~Process() = default;

    /// Runs the process, calling _run()) unless the process is
    /// already running or completed.
    ///
    /// \return whether the process was started or not
    bool run();

    /// Returns the stdin stream pointer.
    ///
    /// \return the stream
    StreamPtr stdIn() const noexcept;

    /// Returns the stdout stream pointer.
    ///
    /// \return the stream
    StreamPtr stdOut() const noexcept;

    /// Returns the stderr stream pointer.
    ///
    /// \return the stream
    StreamPtr stdErr() const noexcept;

    /// Returns whether the process is currently running. This method
    /// should return false if the process has not been started or has
    /// already finished.
    ///
    /// \return whether the process is running
    virtual bool isRunning() = 0;

    /// Returns whether the process has completed its task.
    ///
    /// \return whether the process is finished
    virtual bool isDone() = 0;

    /// Returns the exit code of the process. This method will return
    /// an unspecified value if isDone() is false.
    ///
    /// \return the exit code
    virtual int getExitCode() = 0;

};

/// A process pointer is a shared smart pointer to a Process instance.
using ProcessPtr = std::shared_ptr<Process>;

/// Constructs a new process pointer for a process which will execute
/// the given command. Note that this function may return a null
/// pointer if the operating system it is running on is unknown.
///
/// \param cmd the command
/// \return the process pointer
ProcessPtr makeProcess(std::string cmd);

#endif // PROCESS_HPP
