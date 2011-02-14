#ifndef STRING_UTIL_H
#define STRING_UTIL_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/**
 * An array of strings with an associated length
 */
typedef struct strarray
{
	int dim; /**< Length of the string array */
	char **strings;
} strarray;

/**
 * What is a valid strarray ?
 */
#define is_valid_strarray(x) ((x != (strarray *)NULL) && (x -> dim > 0) && \
							  (x -> strings != (char **)NULL))

extern void free_strarray(strarray *array);
extern strarray *strsplit(char *delim, char *string);
extern int count_tokens(char *delim, char *string);
extern void print_strarray(strarray *array);
extern int remove_quotes(char *string);
int parse_integer(char *string, int *value);
extern int trim(char *string);

#endif /* STRING_UTIL_H */
