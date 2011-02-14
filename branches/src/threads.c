#include <pthread.h>

/**
 * Sets some default pthread attributes
 *
 * @param attr A pthread attribute structure to fill
 */
void default_pthead_attr(pthread_attr_t *attr)
{
	if (attr == (pthread_attr_t *)NULL)
	{
		return; /* Nothing to do */
	}
	pthread_attr_init(attr); /* Initialize the attributes */
	pthread_attr_setdetachstate(attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setstacksize(attr, 1024u * 1024u); /* 1Kb stack size */
	return;
}

