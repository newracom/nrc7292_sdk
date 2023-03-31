#ifndef	SYSTEM_NEW_PRINTF_H
#define SYSTEM_NEW_PRINTF_H

/* printf implementation*/
#define snprintf        snprintf_
#define vsnprintf       vsnprintf_
int  snprintf_(char* buffer, size_t count, const char* format, ...);
int vsnprintf_(char* buffer, size_t count, const char* format, va_list va);

#endif /* SYSTEM_NEW_PRINTF_H */