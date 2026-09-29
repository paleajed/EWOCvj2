#include "logtee.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <shlobj.h>
#include <fcntl.h>
#include <io.h>
#else
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {

// The reader threads are detached and keep running until the OS tears the process down, i.e. also
// during and after static destruction at exit(). Everything they touch is therefore heap-allocated and
// never freed (intentional leak), and they are never joined: child processes that inherited our
// stdout/stderr pipe can keep it open, so the readers would never see end-of-file.
std::mutex &logmutex = *new std::mutex;  // stdout and stderr readers share the log file

#ifdef _WIN32

HANDLE logfile = INVALID_HANDLE_VALUE;

struct TeeStream {
    HANDLE readend = nullptr;
    HANDLE console = nullptr;  // original destination, nullptr if there is none
    std::atomic<bool> busy{false};
    std::atomic<unsigned long long> chunks{0};  // chunks passed on so far, see logtee_flush()
};
TeeStream &teeout = *new TeeStream;  // never freed, see logmutex
TeeStream &teeerr = *new TeeStream;

std::string log_path() {
    char localappdata[MAX_PATH];
    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localappdata))) return "";
    std::string dir = std::string(localappdata) + "\\EWOCvj2";
    CreateDirectoryA(dir.c_str(), nullptr);
    return dir + "\\EWOCvj2.log";
}

void write_all(HANDLE h, const char *data, DWORD len) {
    while (len > 0) {
        DWORD written = 0;
        if (!WriteFile(h, data, len, &written, nullptr) || written == 0) return;
        data += written;
        len -= written;
    }
}

void reader(TeeStream *ts) {
    char buf[4096];
    DWORD got;
    while (ReadFile(ts->readend, buf, sizeof(buf), &got, nullptr) && got > 0) {
        ts->busy = true;
        if (ts->console) write_all(ts->console, buf, got);
        if (logfile != INVALID_HANDLE_VALUE) {
            std::lock_guard<std::mutex> lock(logmutex);
            write_all(logfile, buf, got);
        }
        ts->chunks++;
        ts->busy = false;
    }
}

// The GUI subsystem app has no console of its own. Output goes to the std handle we inherited (IDE run
// window, redirect to file/pipe), else to the console of the terminal we were started from.
HANDLE find_console(DWORD stdhandle) {
    HANDLE h = GetStdHandle(stdhandle);
    if (h && h != INVALID_HANDLE_VALUE && GetFileType(h) != FILE_TYPE_UNKNOWN) {
        // own copy: the CRT fd of the stream owns this same handle, and _dup2() in start_stream closes it
        HANDLE copy = nullptr;
        if (DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &copy, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
            return copy;
        }
    }
    static HANDLE parentconsole = nullptr;
    static bool tried = false;
    if (!tried) {
        tried = true;
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            parentconsole = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        nullptr, OPEN_EXISTING, 0, nullptr);
            if (parentconsole == INVALID_HANDLE_VALUE) parentconsole = nullptr;
        }
    }
    return parentconsole;
}

bool start_stream(TeeStream &ts, FILE *stream, DWORD stdhandle) {
    ts.console = find_console(stdhandle);
    HANDLE writeend;
    if (!CreatePipe(&ts.readend, &writeend, nullptr, 0)) return false;
    // hook the CRT stream to the pipe; in a GUI app the stream may have no fd yet
    int fd = _open_osfhandle((intptr_t)writeend, _O_TEXT);
    if (fd < 0) return false;
    if (_fileno(stream) < 0) freopen("NUL", "w", stream);
    if (_dup2(fd, _fileno(stream)) != 0) return false;
    _close(fd);
    SetStdHandle(stdhandle, (HANDLE)_get_osfhandle(_fileno(stream)));
    setvbuf(stream, nullptr, _IONBF, 0);  // _IOLBF means full buffering in the Windows CRT
    std::thread(reader, &ts).detach();
    return true;
}

void sleep_ms(int ms) { Sleep(ms); }

#else

int logfile = -1;

struct TeeStream {
    int readend = -1;
    int console = -1;  // original destination
    std::atomic<bool> busy{false};
    std::atomic<unsigned long long> chunks{0};  // chunks passed on so far, see logtee_flush()
};
TeeStream &teeout = *new TeeStream;  // never freed, see logmutex
TeeStream &teeerr = *new TeeStream;

std::string log_path() {
    const char *home = getenv("HOME");
    if (!home) return "";
#ifdef __APPLE__
    std::string dir = std::string(home) + "/Library/Logs/EWOCvj2";  // shows up in Console.app
#else
    std::string dir = std::string(home) + "/.ewocvj2";
#endif
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir + "/EWOCvj2.log";
}

void write_all(int fd, const char *data, ssize_t len) {
    while (len > 0) {
        ssize_t written = write(fd, data, len);
        if (written <= 0) return;
        data += written;
        len -= written;
    }
}

void reader(TeeStream *ts) {
    char buf[4096];
    ssize_t got;
    while ((got = read(ts->readend, buf, sizeof(buf))) > 0) {
        ts->busy = true;
        if (ts->console >= 0) write_all(ts->console, buf, got);
        if (logfile >= 0) {
            std::lock_guard<std::mutex> lock(logmutex);
            write_all(logfile, buf, got);
        }
        ts->chunks++;
        ts->busy = false;
    }
}

bool start_stream(TeeStream &ts, FILE *stream, int stdfd) {
    int p[2];
    if (pipe(p) != 0) return false;
    ts.console = dup(stdfd);
    if (ts.console >= 0) fcntl(ts.console, F_SETFD, FD_CLOEXEC);
    fcntl(p[0], F_SETFD, FD_CLOEXEC);  // child processes only get the write end (as their stdout/stderr)
    fflush(stream);
    if (dup2(p[1], stdfd) < 0) return false;
    close(p[1]);
    ts.readend = p[0];
    // a pipe makes stdio fully buffered: line buffering keeps output live and survives most crashes
    setvbuf(stream, nullptr, stream == stderr ? _IONBF : _IOLBF, BUFSIZ);
    std::thread(reader, &ts).detach();
    return true;
}

void sleep_ms(int ms) { usleep(ms * 1000); }

#endif

}  // namespace

void logtee_start() {
    static bool started = false;
    if (started) return;
    started = true;

    std::string path = log_path();
#ifdef _WIN32
    if (!path.empty()) {
        logfile = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    }
    start_stream(teeout, stdout, STD_OUTPUT_HANDLE);
    start_stream(teeerr, stderr, STD_ERROR_HANDLE);
#else
    if (!path.empty()) logfile = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    // a closed terminal must not kill us with SIGPIPE when the reader writes to it
    signal(SIGPIPE, SIG_IGN);
    start_stream(teeout, stdout, STDOUT_FILENO);
    start_stream(teeerr, stderr, STDERR_FILENO);
#endif
    // writes before this point may have hit an invalid stream and left the C++ streams failed
    std::cout.clear();
    std::cerr.clear();
    // drain the pipes on a normal exit(), so the last lines aren't lost
    std::atexit([] { logtee_flush(); });
}

void logtee_flush(int timeout_ms, bool flushstreams) {
    if (flushstreams) {
        std::cout.flush();
        std::cerr.flush();
        fflush(stdout);
        fflush(stderr);
    }
    // Don't ask the pipe how much is still queued: on Windows PeekNamedPipe on the read end blocks
    // behind the reader's pending synchronous ReadFile (I/O on one handle is serialized), which hung
    // exit forever. A reader picks up new data within microseconds, so wait until both readers have
    // been idle for a short quiet period instead - bounded by timeout_ms, never blocking.
    using clock = std::chrono::steady_clock;
    const auto quiet = std::chrono::milliseconds(20);
    auto now = clock::now();
    auto deadline = now + std::chrono::milliseconds(timeout_ms);
    auto quietsince = now;
    unsigned long long lastchunks = teeout.chunks + teeerr.chunks;
    while (now < deadline) {
        sleep_ms(2);
        now = clock::now();
        unsigned long long chunks = teeout.chunks + teeerr.chunks;
        if (teeout.busy || teeerr.busy || chunks != lastchunks) {
            lastchunks = chunks;
            quietsince = now;
        }
        else if (now - quietsince >= quiet) {
            break;
        }
    }
}
