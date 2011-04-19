
#include "chirp_util.h"

/**
 * Attempts to retrieve an integer with the given key from chirp
 *
 * Attempts to retrieve an integer with the given key via chirp. 
 * The output value is placed in ``value''. If an error occurs, 
 * value is left unchanged and the function returns a nonzero 
 * value.
 *
 * @param chirp A connected chirp structure
 * @param key The key to look for
 * @param value (output) The resulting value
 * @return 0 on success, otherwise failure
 */
int get_chirp_integer(struct chirp_client *chirp, const char *key, int *value)
{
	if (chirp == (struct chirp_client *)NULL)
	{
		return 1;
	}
	if (key == (char *)NULL)
	{
		return 2;
	}
	char *char_value = NULL;
	char *next = NULL;
	int str_len = chirp_client_get_job_attr(chirp, key, &char_value);
	if (char_value == NULL)
	{
		return 3;
	}	
	if (str_len <= 0)
	{
		return 4;
	}
	char *new_string = (char *) calloc(str_len + 5, sizeof(char));
	if (new_string == (char *)NULL)
	{
		return 5;
	}
	int i;
	for (i = 0; i < str_len; i++)
	{
		new_string[i] = char_value[i];
	}

	/* Replace any quotation marks */
	remove_quotes(new_string);
	trim(new_string);

	/* Attempt to convert the result into an int */
	errno = 0;
	int new_value = (int) strtol(new_string, &next, 10); /* Base 10 */
	if (errno != 0 || next == new_string || next == NULL)
	{
		free(new_string);
		return 5;
	}
	/* Successfully parsed */
	*value = new_value;
	free(new_string);
	return 0;
}

/**
 * Attempts to retrieve a character string with the given key from chirp
 *
 * Attempts to retrieve a character string with the given key via chirp. 
 * The output value is placed in ``value''. If an error occurs, 
 * value is left unchanged and the function returns a nonzero 
 * value.
 *
 * @param chirp A connected chirp structure
 * @param key The key to look for
 * @return the allocated string on success, otherwise failure
 */
char * get_chirp_string(struct chirp_client *chirp, const char *key)
{
	if (chirp == (struct chirp_client *)NULL)
	{
		return NULL;
	}
	if (key == (char *)NULL)
	{
		return NULL;
	}
	char *char_value = NULL;
	int str_len = chirp_client_get_job_attr(chirp, key, &char_value);
	if (char_value == NULL)
	{
		return NULL;
	}	
	if (str_len <= 0)
	{
		return NULL;
	}
	/* Create a string that is at least 5 characters more than the length of this string */
	char *new_string = (char *) calloc(str_len + 5, sizeof(char));
	if (new_string == (char *)NULL)
	{
		return NULL;
	}
	int i;
	for (i = 0; i < str_len; i++)
	{
		new_string[i] = char_value[i];
	}
	
	/* Replace any quotation marks */
	remove_quotes(new_string);
	trim(new_string);
	/* Check if the value is undefined */
	if (! strncmp("UNDEFINED", new_string, str_len) || 
			! strncmp("undefined", new_string, str_len))
	{
		free(new_string);
		return NULL;
	}
	/* Return the value */
	return new_string;
}
