
#include "Process.hpp"

using namespace std;

/*
 * This is where things start to get a bit messy. Ideally, we'd like to make this as similar
 * to the POSIX system as possible while maintaining support for reasonably modern Windows
 * operating systems as well.
 */

#if defined(__unix__)
#  define USE_POSIX
#elif defined(_WIN32)
#  define USE_WINDOWS
#endif

#ifdef USE_POSIX
#include <mutex>
#include <unistd.h>
#include <sys/wait.h>

class UnixProcess;

// TODO Perhaps add a flush function to force flushing of output and bypass buffering
//      (Would have to do this in Stream.hpp as well)
class FilePtrStream : public Stream {
private:
    int fd;
    bool isIn;
    bool isOut;
    bool eof;
public:
    FilePtrStream(int file, bool in, bool out)
        : fd(file), isIn(in), isOut(out), eof(false) {}
    virtual ~FilePtrStream() {
        close();
    }
    virtual bool hasOut() {
        return isOut;
    }
    virtual bool hasIn() {
        return isIn;
    }
    virtual void out(char ch) {
        write(fd, &ch, 1);
    }
    virtual char in() {
        char ch;
        if (read(fd, &ch, 1) == 0)
            eof = true;
        return ch;
    }
    virtual bool isEof() {
        return eof;
    }
    virtual void close() {
        ::close(fd);
    }
};

class StdMutex : public Mutex {
private:
    mutex impl;
public:
    virtual void lock() {
        impl.lock();
    }
    virtual void unlock() {
        impl.unlock();
    }
};

class UnixProcess : public Process {
private:
    int pid;
    bool flagged;
    bool done;
    int exitCode;
    virtual int _run();
    void refreshCodes();
public:
    UnixProcess(string cmd)
        : Process(cmd) , pid(0), flagged(false), done(false), exitCode(-1) {}
    virtual bool isRunning() {
        refreshCodes();
        return (pid != 0) && (!done);
    }
    virtual bool isDone() {
        refreshCodes();
        return (pid != 0) && (done);
    }
    virtual int getExitCode() {
        refreshCodes();
        return exitCode;
    }
};

void UnixProcess::refreshCodes() {
    if (!flagged) {
        if (pid != 0) {
            int status;
            int result = waitpid(pid, &status, WNOHANG);
            if (result == 0) {
                // Still alive; don't flag
            } else if (result == -1) {
                // Error; flag and stop
                flagged = true;
                done = true;
                exitCode = -1;
            } else {
                // Terminated; flag and stop
                flagged = true;
                done = true;
                exitCode = status;
            }
        }
    }
}

int UnixProcess::_run() {
    constexpr int READ = 0;
    constexpr int WRITE = 1;
    int sIn[2];
    int sOut[2];
    int sErr[2];
    if (pipe(sIn) < 0)
        return 1;
    if (pipe(sOut) < 0)
        return 1;
    if (pipe(sErr) < 0)
        return 1;
    int pid_ = fork();
    if (pid_ == 0) {
        // We are in the child process
        dup2(sIn[READ], STDIN_FILENO);
        dup2(sOut[WRITE], STDOUT_FILENO);
        dup2(sErr[WRITE], STDERR_FILENO);

        close(sIn[READ]);
        close(sIn[WRITE]);
        close(sOut[READ]);
        close(sOut[WRITE]);
        close(sErr[READ]);
        close(sErr[WRITE]);

        int code = system(cmd.c_str());

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        exit(code);

    } else if (pid_ < 0) {
        return 1; // Something bad happened
    } else {
        // Close the unused buffers
        close(sIn[READ]);
        close(sOut[WRITE]);
        close(sErr[WRITE]);

        this->in  = StreamPtr( new FilePtrStream(sIn [WRITE], false, true ) );
        this->out = StreamPtr( new FilePtrStream(sOut[READ ], true , false) );
        this->err = StreamPtr( new FilePtrStream(sErr[READ ], true , false) );
        this->pid = pid_;

    }
    return 0;
}

ProcessPtr makeProcess(string cmd) {
    return ProcessPtr(new UnixProcess(cmd));
}

#endif // USE_POSIX

#ifdef USE_WINDOWS
// TODO This
#endif // USE_WINDOWS

Process::Process(std::string cmd)
    : cmd(cmd), in(new NullStream()), out(new NullStream()), err(new NullStream()) {}

bool Process::run() {
    if ((!this->isDone()) && (!this->isRunning())) {

        int code = this->_run();
        if (code != 0)
            return false;
        return true;

    }
    return false;
}

StreamPtr Process::stdIn() const noexcept {
    return in;
}

StreamPtr Process::stdOut() const noexcept {
    return out;
}

StreamPtr Process::stdErr() const noexcept {
    return err;
}
