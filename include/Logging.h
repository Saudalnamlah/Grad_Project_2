#pragma once
#include <cstdio>
#include <cstdarg>
#include <mutex>

enum class LogLevel { Info, Warn, Error, Debug };

inline void log_msg(LogLevel lvl, const char* fmt, ...) {
  static std::mutex m;
  std::lock_guard<std::mutex> lock(m);

  const char* p = "INFO";
  if (lvl == LogLevel::Warn) p = "WARN";
  if (lvl == LogLevel::Error) p = "ERR ";
  if (lvl == LogLevel::Debug) p = "DBG ";

  std::fprintf(stderr, "[%s] ", p);
  va_list args;
  va_start(args, fmt);
  std::vfprintf(stderr, fmt, args);
  va_end(args);
  std::fprintf(stderr, "\n");
}
