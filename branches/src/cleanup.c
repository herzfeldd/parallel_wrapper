
#include "wrapper.h"
#include "scratch.h"
#include <signal.h>
#include <setjmp.h>
int exit_flag = 0;

/**
 * Signal handler (SIGINT, SIGTERM, SIGHUP...)
 */
void handle_exit_signal(int signal) 
{
	print(PRNT_INFO, "Received signal %d\n", signal);
	exit_flag = 1;
	if (jmpset)
	{	
		siglongjmp(jmpbuf, 1);
	}
	else
	{
		exit(signal);
	}
}

/**
 * Clean up and then exit with the associated return code
 */
void cleanup(parallel_wrapper *par_wrapper, int return_code)
{
	int i, j;
	if (par_wrapper -> this_machine -> rank == MASTER && 
			par_wrapper -> machines != (machine **)NULL)
	{

		/* Send the term signal (don't send to self) */
		for (i = 1; i < par_wrapper -> num_procs; i++)
		{
			if (par_wrapper -> machines[i] == (machine *)NULL)
			{
				continue;
			}
			for (j = 0; j < 10; j++)
			term(par_wrapper -> command_socket, return_code, 
					par_wrapper -> machines[i] -> ip_addr, par_wrapper -> machines[i] -> port);
		}
	}

	/* Lock the parallel_wrapper structure */
	pthread_mutex_lock(&par_wrapper -> mutex);

	/* Clean up the scratch directory */
	cleanup_scratch(par_wrapper -> scratch_dir);

	/* Unlink all softlinks */
	if (is_valid_sll(par_wrapper -> symlinks))
	{
		struct sll_element *element = par_wrapper -> symlinks -> head -> next;
		while (element != (struct sll_element *)NULL)
		{
			char *filename = (char *)element -> ptr;
			if (filename != (char *)NULL)
			{
				if (unlink(filename) != 0)
				{
					print(PRNT_WARN, "Unable to unlink file %s\n", filename);
				}
			}
			element = element -> next;
		}
	}
	/* No need to unlock - we are exitting */
	exit(return_code);
}
