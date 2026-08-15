#pragma once
#include <string>
#include <sstream>

// Lightweight logging to a file (game.log) + console, with a crash handler that records the
// signal and a backtrace before the process dies. The point is bug reports: when something goes
// wrong the user can attach game.log and it says what the game was doing and where it crashed.
namespace Log {
    enum class Level { Info, Warn, Error };

    void init(const std::string& path = "game.log"); // truncates; keeps the previous run as .prev
    void write(Level level, const std::string& msg);
    void installCrashHandlers();                      // SIGSEGV/SIGABRT/SIGFPE + std::terminate
    std::string demangle(const char* typeidName);     // readable type name for logs
}

// Stream-style: LOG_INFO("opening match " << home << " vs " << away);
#define LOG_INFO(expr)  do { std::ostringstream _l; _l << expr; ::Log::write(::Log::Level::Info,  _l.str()); } while (0)
#define LOG_WARN(expr)  do { std::ostringstream _l; _l << expr; ::Log::write(::Log::Level::Warn,  _l.str()); } while (0)
#define LOG_ERROR(expr) do { std::ostringstream _l; _l << expr; ::Log::write(::Log::Level::Error, _l.str()); } while (0)
