/**
 * Singly linked list implementation
 */

#include "sll.h"
#include <stdlib.h>
/**
 * Get an empty singly linked list
 *
 * @return An initialized sl_list or NULL if error
 */
sl_list *sll_get_list(void)
{
	sl_list *list = (sl_list *)calloc(1, sizeof(struct sl_list));
	if (list == (sl_list *)NULL)
	{
		return NULL;
	}
	/* Allocate the head */
	list -> head = (struct sll_element *)calloc(1, sizeof(struct sll_element));
	if (list -> head == (struct sll_element *)NULL)
	{
		free(list);
		return NULL;
	}

	return list;
}

/**
 * Add an element to a singly linked list
 *
 * Adds an element to the head of a singly linked list.
 *
 * @param list The list to add to
 * @param element The element to add to the list
 */
int sll_add_element(sl_list *list, void *ptr)
{
	if (! is_valid_sll(list))
	{
		return 1;
	}	
	/* Allocate a new element */
	struct sll_element *new_element = (struct sll_element *)calloc(1, sizeof(struct sll_element));
	if (new_element == (struct sll_element *)NULL)
	{
		return 2;
	}
	new_element -> ptr = ptr;
	/* Add to head of the list */
	new_element -> next = list -> head -> next;
	list -> head -> next = new_element;
	return 0;
}
