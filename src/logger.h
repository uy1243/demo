#pragma once
#include <cstdio>
#include <cstdarg>
#include <time.h>

#define LOG_INFO(format, ...) log_write("INFO", format, ##__VA_ARGS__)
#define LOG_ERR(format, ...)  log_write("ERROR", format, ##__VA_ARGS__)

void log_write(const char* level, const char* format, ...);