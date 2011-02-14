#ifndef TIMER_H
#define TIMER_H
#include <time.h>
#include <stdlib.h>
/**
 * A basic timer structure
 */
typedef struct timer
{
	time_t start;
	struct timeval delay;
	struct timer *next;
} timer;

typedef struct timer_list
{
	struct timer *head;
	int num_timers;
} timer_list;

extern timer *new_timer(long int sec, long int usec);
extern struct timeval time_to_next_firing(timer *x);
extern int add_timer(timer_list *list, timer *x);
extern int remove_timer(timer_list *list, timer *x);
extern int next_timer(timer_list *list, struct timeval *timeval);
#endif /* TIMER_H */
