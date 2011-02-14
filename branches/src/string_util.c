
#include "string_util.h"
#include "log.h"

/**
 * Free a STRING_ARRAY structure
 *
 * @param array The string array to free
 */
void free_strarray(strarray *array)
{
	int i;
	if (array == (strarray *)NULL)
	{
		return; /* Nothing to do */
	}
	if (array -> strings == (char **)NULL)
	{
		free(array); 
		return;
	}
	/* OK, everything is initialized, free it all */
	for (i = 0; i < array -> dim; i++)
	{
		if (array -> strings[i] != (char *)NULL)
		{
			free(array -> strings[i]);
		}
	}
	free(array -> strings);
	free(array);
	return;
}

/**
 * Prints and strarray to stdout
 *
 * @param array The strarray to pass
 */
void print_strarray(strarray *array)
{
	int i;
	if (!is_valid_strarray(array))
	{
		return;
	}
	for (i = 0; i < array -> dim; i++)
	{
		printf("%s ", array -> strings[i]);
	}
	printf("\n");
	return;
}

/**
 * Split a string using the passed delimiters
 *
 * Split a string using an array of passed character delimiters.
 * If the delimiters array is NULL, a default set of delimiters is
 * used (space, tab, newline). The results of the split are returned
 * in an allocated strarray structure
 *
 * @param delim A character array of delimiters to use
 * @param string The string to split
 * @return an strarray on success otherwise NULL
 */
strarray *strsplit(char *delim, char *string)
{
	int i, j;
	strarray *array = NULL;
	if (string == (char *)NULL)
	{
		return NULL; /* Nothing to do */
	}
	/* First, determine the number of substrings */
	int tokens = count_tokens(delim, string);
	if (tokens <= 0)
	{
		return NULL;
	}
	/* Allocate the space for all of the tokens */
	array = (struct strarray *) malloc(sizeof(struct strarray));
	if (array == (strarray *)NULL)
	{
		return NULL;	
	}
	array -> strings = (char **) calloc(tokens, sizeof(char *));
	if (array -> strings == (char **)NULL)
	{
		free_strarray(array);
		return NULL;
	}
	array -> dim = tokens;
	/* Get the resulting tokens */
	int current_token = 0;
	int start_index = 0;
	int end_index = 1;
	int string_length = strlen(string);
	int delim_length = strlen(delim);
	int last_was_delim = 0;
	for (i = 0; i < string_length; i++)
	{
		int found_delim = 0;
		for (j = 0; j < delim_length; j++)
		{
			if (string[i] == delim[j])
			{
				/* Found a delimiter */
				found_delim = 1;
				break;
			}
		}
		if (found_delim && !last_was_delim)
		{
			end_index = i;
			array -> strings[current_token] = (char *) calloc(end_index - start_index + 2, sizeof(char));
			if (array -> strings[current_token] == (char *)NULL)
			{
				free_strarray(array);
				return NULL;
			}
			strncpy(array -> strings[current_token], string + start_index, end_index - start_index);	
			last_was_delim = 1;
			current_token++;
			start_index = i+1;
		}
		else if (found_delim && last_was_delim)
		{
			last_was_delim = 1;
			start_index = i+1;
		}
		else
		{
			/* We didn't find anything - last was not a delimiter */
			last_was_delim = 0;
		}
	}
	if (!last_was_delim)
	{
		/* Get the last token */
		end_index = string_length;
		array -> strings[current_token] = (char *) calloc(end_index - start_index + 2, sizeof(char));
		if (array -> strings[current_token] == (char *)NULL)
		{
			free_strarray(array);
			return NULL;
		}
		strncpy(array -> strings[current_token], string + start_index, end_index - start_index);
	}
	return array;
}

/**
 * Counts the number of tokens given a set of delimiters
 *
 * Counts the number of tokens in a passed string using a
 * set of delimiters. Consecutive delimiters are ignored in the
 * count of the total number of tokens
 *
 * @param delim A character array of delimiters to use
 * @param string The string to tokenize
 * @return The number of tokens in the string (negative value if error)
 */
int count_tokens(char *delim, char *string)
{
	int i, j; /* Counters */
	int tokens = 1; /* We always start with one token */
	if (delim == (char *)NULL || string == (char *)NULL)
	{
		return -1; /* Error */
	}
	int string_length = strlen(string);
	int delim_length = strlen(delim);
	int last_was_delim = 0;
	for (i = 0; i < string_length; i++)
	{
		int found_delim = 0;
		for (j = 0; j < delim_length; j++)
		{
			if (string[i] == delim[j])
			{
				/* Found a delimiter */
				found_delim = 1;
				break;
			}
		}
		if (found_delim && !last_was_delim)
		{
			tokens++;
			last_was_delim = 1;
		}
		else if (found_delim && last_was_delim)
		{
			last_was_delim = 1;
		}
		else
		{
			/* We didn't find anything - last was not a delimiter */
			last_was_delim = 0;
		}
	}
	if (last_was_delim)
	{
		tokens--;
	}	
	return tokens;
}