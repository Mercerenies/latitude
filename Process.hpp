#ifndef PROCESS_HPP
#define PROCESS_HPP

#include "Stream.hpp"
#include <string>

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

#endif // PROCESS_HPP
