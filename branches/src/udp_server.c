#include "wrapper.h"

/**
 * Starts a UDP server
 *
 * Starts a UDP server on a UDP port. The port is chosen within
 * the range specified in the parallel_wrapper
 *
 * @ptr a void pointer which contains a parallel_wrapper
 * @return Nothing
 */
void *udp_server(void *ptr)
{
	int RC;
	parallel_wrapper *par_wrapper = (parallel_wrapper *) ptr;
	if (par_wrapper == (parallel_wrapper *)NULL)
	{
		print(PRNT_ERR, "Invalid parallel_wrapper\n");
		return NULL;
	}

	/* Get an open UDP port */
	RC = get_bound_dgram_socket_by_range(par_wrapper -> low_port, 
		par_wrapper -> high_port, &par_wrapper -> this_machine -> port, 
		&par_wrapper -> command_socket);		
	if (RC != 0)
	{
		print(PRNT_ERR, "Unable to bind to command socket\n");
		return NULL;
	}
	else
	{
		debug(PRNT_INFO, "Bound to command port: %d\n", par_wrapper -> this_machine -> port);
	}

	return NULL;
}


