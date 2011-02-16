#ifndef SLL_H
#define SLL_H

struct sll_element 
{
	struct sll_element *next;
	void *ptr;
} ;

typedef struct sl_list
{
	struct sll_element *head;
	int num_elements;
} sl_list;

#define is_valid_sll(x) (x != NULL && x -> head != NULL && x -> head -> ptr == NULL)

extern sl_list *sll_get_list(void);
extern int sll_add_element(sl_list *, void *);

#endif /* SLL_H */
