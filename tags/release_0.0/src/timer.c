#include "timer.h"
#include "log.h"
#include <stdio.h>
/**
 * Returns a time with the defined periodicity
 *
 * Creates a new time whose base is the current time. The timer should
 * fire with a period specified by the combination of sec + usec. 
 * 
 * @param sec The periodicity on the order of seconds
 * @param usec The periodicity on the order of microseconds
 * @return A timer whose basetime is the current time or NULL if 
 *   there is an error
 */
timer *new_timer(long int sec, long int usec)
{
	/* Allocate space for a new timer */
	timer *new_timer = (timer *) calloc(1, sizeof(struct timer));
	if (new_timer == (timer *)NULL)
	{
		print(PRNT_ERR, "Unable to allocate space for a new timer\n");
		return NULL;
	}
	new_timer -> delay.tv_sec = sec;
	new_timer -> delay.tv_usec = usec;
	new_timer -> next = NULL;
	/* Get the current time */
	gettimeofday(&new_timer -> start, NULL);
	return new_timer;
}

/**
 * Returns a timeval structure containing dt until the next firing
 *
 * Returns a timeval structure noting the time until timer x fires
 * again. Missed timings are not noted. Therefore, the returned 
 * timeval structure will always be positive
 *
 * @param x an initialized timer
 * @return a timeval structure containing the time of next firing
 */
struct timeval time_to_next_firing(timer *x)
{
	struct timeval curr_time;
	struct timeval next_firing;
	next_firing.tv_usec = 0;
	next_firing.tv_sec = 0;
	if (x == (timer *)NULL)
	{
		return next_firing; /* Can't really do anything */
	}
	/* Get the current time */
	gettimeofday(&curr_time, NULL);
	/* Subtract the two times */
	struct timeval diff_time;
	timersub(&curr_time, &x -> start, &diff_time);
	long int remaining_time = (diff_time.tv_usec + diff_time.tv_sec * 1000000) % 
		(x -> delay.tv_usec + x -> delay.tv_sec * 1000000);
	/* Place back in a timeval structure */
	next_firing.tv_usec = (remaining_time % 1000000);
	next_firing.tv_sec = (remaining_time / 1000000);
	timersub(&x-> delay, &next_firing, &next_firing);
	return next_firing;
}

/**
 * Add a timer to the SLL
 */
int add_timer(timer_list *list, timer *x)
{
	if (x == (timer *)NULL)
	{
		return 1;
	}
	if (list == (timer_list *)NULL)
	{
		return 2;
	}
	x -> next = list -> head;
	list -> head = x;
	list -> num_timers++;
	return 0;
}

/**
 * Remove a timer from the SLL
 */
int remove_timer(timer_list *list, timer *x)
{
	/* Traverse the list looking for the timer x */
	if (list == (timer_list *)NULL)
	{
		return 1;
	}
	if (x == (timer *)NULL)
	{
		return 2;
	}
	timer *curr_timer = list -> head;
	timer *prev_timer = NULL;
	while (curr_timer != NULL)
	{
		if (curr_timer == x)
		{
			if (prev_timer != NULL)
			{
				prev_timer -> next = curr_timer -> next;
			}
			else
			{
				list -> head = NULL;
			}
			free(curr_timer);
			list -> num_timers--;
			return 0;
		}
		curr_timer = curr_timer -> next;
	}
	return 3; /* Couldn't find it */
}

/**
 * Get next timer firing
 */
int next_timer(timer_list *list, struct timeval *next_timer)
{
	if(list == (timer_list *)NULL)
	{
		return 1;
	}			
	if (next_timer == (struct timeval *)NULL)
	{
		return 2;
	}
	if (list -> num_timers == 0)
	{
		return 3; /* Empty list */
	}
	long int min_time = 0;
	struct timeval next_firing = time_to_next_firing(list -> head);
	min_time = next_firing.tv_usec + next_firing.tv_sec * 1000000;
	
	timer *curr = list -> head -> next;
	while (curr != (timer *)NULL)
	{
		next_firing = time_to_next_firing(curr);
		long int next_time = next_firing.tv_usec + next_firing.tv_sec * 1000000;
		if (next_time < min_time)
		{
			min_time = next_time;
		}
		curr = curr -> next;
	}
	next_timer -> tv_usec = min_time % 1000000;
	next_timer -> tv_sec = min_time / 1000000;
	return 0;
}

