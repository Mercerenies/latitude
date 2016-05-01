#ifndef _PROCESS_HPP_
#define _PROCESS_HPP_

#include "Stream.hpp"
#include <string>

class Mutex {
public:
    virtual void lock() = 0;
    virtual void unlock() = 0;
};

class Process {
protected:
    std::string cmd;
    StreamPtr in;
    StreamPtr out;
    StreamPtr err;
    virtual int _run() = 0;
public:
    Process(std::string cmd);
    virtual ~Process() = default;
    bool run(); // Returns whether successful
    StreamPtr stdIn() const noexcept;
    StreamPtr stdOut() const noexcept;
    StreamPtr stdErr() const noexcept;
    virtual bool isRunning() = 0;
    virtual bool isDone() = 0;
    virtual int getExitCode() = 0;
};

using ProcessPtr = std::shared_ptr<Process>;

ProcessPtr makeProcess(std::string);

#endif // _PROCESS_HPP_
