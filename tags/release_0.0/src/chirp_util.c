
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
	if (str_len == 0)
	{
		return 4;
	}
	/* Insert a null character into the string */
	char_value[str_len] = '\0';
	/* Replace any quotation marks */
	remove_quotes(char_value);
	trim(char_value);
	/* Attempt to convert the result into an int */
	errno = 0;
	int new_value = (int) strtol(char_value, &next, 10); /* Base 10 */
	if (errno != 0 || next == char_value || next == NULL)
	{
		return 5;
	}
	/* Successfully parsed */
	*value = new_value;
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
 * @param value (output) The resulting value
 * @return 0 on success, otherwise failure
 */
int get_chirp_string(struct chirp_client *chirp, const char *key, char **value)
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
	int str_len = chirp_client_get_job_attr(chirp, key, &char_value);
	if (char_value == NULL)
	{
		return 3;
	}	
	if (str_len == 0)
	{
		return 4;
	}
	/* Insert a null character into the string */
	char_value[str_len] = '\0';
	/* Replace any quotation marks */
	remove_quotes(char_value);
	trim(char_value);
	/* Check if the value is undefined */
	if (! strncmp("UNDEFINED", char_value, str_len) || 
			! strncmp("undefined", char_value, str_len))
	{
		return 5;
	}
	/* Return the value */
	*value = strdup(char_value);
	return 0;
}
