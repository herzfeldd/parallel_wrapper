/**
 * Logging utilities
 */

#define _GNU_SOURCE
#include "log.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Define functions for printing (both in the debug case and the non-debug case) */
void print_message(enum PRINT_LEVEL type,  char *file, int line, __const char *__restrict __format, ...)
{
	time_t raw_time;
	struct tm * time_info;
	time (&raw_time);
	time_info = localtime(&raw_time);
	char *current_time = asctime(time_info);
	/* Remove the mandatory \n at the end of the message */
	int str_length = strlen(current_time);
	if (str_length >= 2)
	{
		current_time[str_length - 6] = '\0';
	}
	else
	{
		type = -1; /* Invalid type */
	}

	/* NOTE: We assume that the message contains the return character */
	switch (type)
	{
		case PRNT_ERR:
			fprintf(stderr, "%s ERROR in %s:%d: ", current_time, file, line);
			break;

		case PRNT_WARN:
			fprintf(stderr, "%s WARN in %s:%d: ", current_time, file, line);
			break;

		case PRNT_INFO:
			fprintf(stdout, "%s INFO: ", current_time);
			break;
		default:
			fprintf(stdout, "INFO: ");
			break;
	}	
	va_list args;
	va_start(args, __format);
	if (type == PRNT_ERR || type == PRNT_WARN)
	{
		vfprintf(stderr, __format, args);
	}
	else
	{
		/* Print to stdout */
		vfprintf(stdout, __format, args);
	}
	va_end(args);
}

/* Print the message only if debugging is enabled
 * NOTE: There must be some environment variable 'PARALLEL_DEBUG'
 * that, if defined, we output values
 */
void debug_message(enum PRINT_LEVEL type,  char *file, int line, __const char *__restrict __format, ...)
{
	int RC; /* generic function return code */

	char *debug_val;
	debug_val = getenv ("PARALLEL_DEBUG");
	if (debug_val != NULL || type == PRNT_ERR)
	{
		va_list args;
		va_start(args, __format);
		char *expanded_string;
		RC = vasprintf(&expanded_string, __format, args);
		print_message(type, file, line, expanded_string);
		free(expanded_string);
		va_end(args);
	}
}
