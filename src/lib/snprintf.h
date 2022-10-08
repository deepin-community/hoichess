#ifndef SNPRINTF_H
#define SNPRINTF_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {

extern int pg_vsnprintf(char *str, size_t count, const char *fmt, va_list args);
extern int pg_snprintf(char *str, size_t count, const char *fmt, ...);
extern int pg_sprintf(char *str, const char *fmt, ...);
extern int pg_fprintf(FILE *stream, const char *fmt, ...);
extern int pg_printf(const char *fmt, ...);

}

#define vsnprintf pg_vsnprintf
#define snprintf pg_snprintf

#endif // SNPRINTF_H
