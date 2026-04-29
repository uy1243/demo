#include "Logger.h"

void log_write(const char* level, const char* format, ...) {
    char buf[1024];
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);

    snprintf(buf, sizeof(buf),
        "[%04d-%02d-%02d %02d:%02d:%02d] [%s] ",
        tm->tm_year + 1900,
        tm->tm_mon + 1,
        tm->tm_mday,
        tm->tm_hour,
        tm->tm_min,
        tm->tm_sec,
        level
    );

    va_list ap;
    va_start(ap, format);
    vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), format, ap);
    va_end(ap);

    printf("%s\n", buf);
}