#ifndef LOG_H
#define LOG_H

/* Define some constants for printing */
typedef enum PRINT_LEVEL
{
	PRNT_ERR = 1,
	PRNT_WARN = 2,
	PRNT_INFO = 3
} PRINT_LEVEL;

#define print(type, format, ...) print_message(type, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define debug(type, format, ...) debug_message(type, __FILE__, __LINE__, format, ##__VA_ARGS__)

/**
 * Print a DEBUG message 
 */
extern void print_message(enum PRINT_LEVEL type,  char *file, int line, __const char *__restrict __format, ...);

/*
 * Define functions for printing (both in the debug case 
 * and the non-debug case) 
 */
extern void debug_message(enum PRINT_LEVEL type,  char *file, int line, __const char *__restrict __format, ...);

#endif /* Define ERR_H */
