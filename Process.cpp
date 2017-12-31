
#include "Process.hpp"
#include "Platform.hpp"

using namespace std;

#ifdef USE_POSIX
#include <mutex>
#include <unistd.h>
#include <sys/wait.h>

class UnixProcess;

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
    virtual bool hasOut() const noexcept {
        return isOut;
    }
    virtual bool hasIn() const noexcept {
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
    virtual bool isEof() const noexcept {
        return eof;
    }
    virtual void close() {
        ::close(fd);
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
#define WINVER 0x0500
#include <windows.h>

class HandleStream : public Stream {
private:
    HANDLE handle;
    bool isIn;
    bool isOut;
    bool eof;
public:
    HandleStream(HANDLE file, bool in, bool out)
        : handle(file), isIn(in), isOut(out), eof(false) {}
    virtual ~HandleStream() {
        close();
    }
    virtual bool hasOut() const noexcept {
        return isOut;
    }
    virtual bool hasIn() const noexcept {
        return isIn;
    }
    virtual void out(char ch) {
        DWORD ignore; // The system requires that this be non-null for some reason
        WriteFile(handle, &ch, 1, &ignore, NULL);
    }
    virtual char in() {
        DWORD result;
        char ch;
        ReadFile(handle, &ch, 1, &result, NULL);
        if (result == 0)
            eof = true;
        return ch;
    }
    virtual bool isEof() const noexcept {
        return eof;
    }
    virtual void close() {
        CloseHandle(handle);
    }
};

class WindowsProcess : public Process {
private:
    HANDLE handle;
    bool flagged;
    bool done;
    int exitCode;
    virtual int _run();
    void refreshCodes();
public:
    WindowsProcess(string cmd)
        : Process(cmd), handle(nullptr), flagged(false), done(false), exitCode(-1) {}
    virtual ~WindowsProcess() {
        if (handle)
            CloseHandle(handle);
    }
    virtual bool isRunning() {
        refreshCodes();
        return (handle) && (!done);
    }
    virtual bool isDone() {
        refreshCodes();
        return (handle) && (done);
    }
    virtual int getExitCode() {
        refreshCodes();
        return exitCode;
    }
};

void WindowsProcess::refreshCodes() {
    if (!flagged) {
        if (handle) {
            DWORD result = WaitForSingleObject(handle, 0);
            if (result == WAIT_OBJECT_0) {
                // Done; handle the flag
                DWORD status;
                GetExitCodeProcess(handle, &status);
                flagged = true;
                done = true;
                exitCode = status;
            }
        }
    }
}

typedef std::tuple<HANDLE, HANDLE, HANDLE> HandleTriple;

void CALLBACK waitCallback(PVOID param, BOOLEAN fired) {
    HandleTriple* triple = (HandleTriple*)param;
    CloseHandle(get<0>(*triple));
    CloseHandle(get<1>(*triple));
    CloseHandle(get<2>(*triple));
    delete triple;
}

int WindowsProcess::_run() {
    HANDLE inPipeRd, inPipeWr, outPipeRd, outPipeWr, errPipeRd, errPipeWr;
    SECURITY_ATTRIBUTES attr;
    PROCESS_INFORMATION proc;
    STARTUPINFO start;

    attr.nLength = sizeof(attr);
    attr.bInheritHandle = TRUE;
    attr.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&inPipeRd, &inPipeWr, &attr, 0))
        return 1;
    if (!CreatePipe(&outPipeRd, &outPipeWr, &attr, 0))
        return 1;
    if (!CreatePipe(&errPipeRd, &errPipeWr, &attr, 0))
        return 1;
    if (!SetHandleInformation(inPipeWr, HANDLE_FLAG_INHERIT, 0))
        return 1;
    if (!SetHandleInformation(outPipeRd, HANDLE_FLAG_INHERIT, 0))
        return 1;
    if (!SetHandleInformation(errPipeRd, HANDLE_FLAG_INHERIT, 0))
        return 1;

    ZeroMemory(&proc, sizeof(proc));
    ZeroMemory(&start, sizeof(start));

    start.cb = sizeof(start);
    start.hStdError = errPipeWr;
    start.hStdOutput = outPipeWr;
    start.hStdInput = inPipeRd;
    start.dwFlags = STARTF_USESTDHANDLES;

    string temp(cmd);
    char* buffer = new char[temp.size() + 1];
    strcpy(buffer, temp.c_str());
    BOOL success = CreateProcess(nullptr, buffer,
                                 nullptr, nullptr,
                                 TRUE,
                                 0,
                                 nullptr, nullptr,
                                 &start, &proc);
    delete[] buffer;

    if (!success)
        return 1;

    HANDLE wait;
    RegisterWaitForSingleObject(&wait, proc.hProcess, &waitCallback,
                                new HandleTriple(inPipeRd, outPipeWr, errPipeWr),
                                INFINITE, WT_EXECUTEONLYONCE);

    CloseHandle(proc.hThread);
    this->handle = proc.hProcess;

    this->in  = StreamPtr( new HandleStream( inPipeWr, false, true ) );
    this->out = StreamPtr( new HandleStream(outPipeRd, true , false) );
    this->err = StreamPtr( new HandleStream(errPipeRd, true , false) );

    return 0;

}

ProcessPtr makeProcess(string cmd) {
    return ProcessPtr(new WindowsProcess(cmd));
}

#endif // USE_WINDOWS

#ifdef USE_NULL

ProcessPtr makeProcess(string cmd) {
    return ProcessPtr();
}

#endif // USE_NULL

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
