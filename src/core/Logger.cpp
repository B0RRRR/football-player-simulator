#include "Logger.h"
#include <fstream>
#include <iostream>
#include <mutex>
#include <ctime>
#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <cstring>

#if defined(__GNUG__)
#include <cxxabi.h>
#endif
#if defined(__linux__)
#include <execinfo.h>
#include <unistd.h>
#endif

namespace {
    std::ofstream g_file;
    std::mutex g_mtx;

    std::string timestamp() {
        std::time_t t = std::time(nullptr);
        std::tm lt{};
#if defined(_WIN32)
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        char b[16];
        std::strftime(b, sizeof(b), "%H:%M:%S", &lt);
        return b;
    }

    const char* levelName(Log::Level l) {
        return l == Log::Level::Error ? "ERROR" : l == Log::Level::Warn ? "WARN " : "INFO ";
    }

    void dumpBacktrace() {
#if defined(__linux__)
        void* frames[64];
        int n = backtrace(frames, 64);
        char** syms = backtrace_symbols(frames, n);
        if (g_file.is_open()) {
            g_file << "--- backtrace (" << n << " frames) ---\n";
            for (int i = 0; i < n; ++i) g_file << "  " << (syms ? syms[i] : "?") << "\n";
            g_file.flush();
        }
        std::free(syms);
#endif
    }

    const char* signalName(int sig) {
        switch (sig) {
            case SIGSEGV: return "SIGSEGV (segfault)";
            case SIGABRT: return "SIGABRT (abort)";
            case SIGFPE:  return "SIGFPE (fp/div-by-zero)";
#if !defined(_WIN32)
            case SIGBUS:  return "SIGBUS";
#endif
            default:      return "signal";
        }
    }

    void signalHandler(int sig) {
        Log::write(Log::Level::Error, std::string("CRASH: caught ") + signalName(sig) +
                                          " (" + std::to_string(sig) + ")");
        dumpBacktrace();
        std::signal(sig, SIG_DFL); // restore default and re-raise so the OS still produces a core
        std::raise(sig);
    }

    void terminateHandler() {
        Log::write(Log::Level::Error, "CRASH: std::terminate (uncaught exception)");
        dumpBacktrace();
        std::abort();
    }
}

void Log::init(const std::string& path) {
    std::rename(path.c_str(), (path + ".prev").c_str()); // keep the previous run's log
    g_file.open(path, std::ios::trunc);
    write(Level::Info, "==== session start ====");
}

void Log::write(Level level, const std::string& msg) {
    std::lock_guard<std::mutex> lk(g_mtx);
    std::string line = "[" + timestamp() + "] " + levelName(level) + " " + msg;
    if (g_file.is_open()) { g_file << line << "\n"; g_file.flush(); }
    (level == Level::Error ? std::cerr : std::cout) << line << std::endl;
}

void Log::installCrashHandlers() {
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGFPE, signalHandler);
#if !defined(_WIN32)
    std::signal(SIGBUS, signalHandler);
#endif
    std::set_terminate(terminateHandler);
}

std::string Log::demangle(const char* name) {
#if defined(__GNUG__)
    int status = 0;
    char* d = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    std::string r = (status == 0 && d) ? d : name;
    std::free(d);
    return r;
#else
    return name ? name : "";
#endif
}
