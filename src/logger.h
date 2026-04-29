#pragma once
#include <cstdio>
#include <cstdarg>
#include <time.h>
#include <cstring>

#define LOG_INFO(...) log_write("INFO", __VA_ARGS__)
#define LOG_ERR(...) log_write("ERROR", __VA_ARGS__)

void log_write(const char* level, const char* format, ...);