#include "wrapper.h"

void cleanup(int RC, PARALLEL_WRAPPER *par_wrapper)
{
	if (par_wrapper -> chirp != (struct chirp_client *)NULL)
	{
		chirp_client_disconnect(par_wrapper -> chirp);
	}
	exit(RC);
}
