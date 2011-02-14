
#include "wrapper.h"
#include "chirp_util.h"

int main(int argc, char **argv)
{
	int RC;
	pthread_attr_t attr;
	default_pthead_attr(&attr);

	/* Allocate a new parallel_wrapper structure */
	parallel_wrapper *par_wrapper = (parallel_wrapper *)calloc(1, sizeof(struct parallel_wrapper));
	if (par_wrapper == (parallel_wrapper *)NULL)
	{
		print(PRNT_ERR, "Unable to allocate space for parallel wrapper\n");
		return 1;
	}

	/* Create structures for this machine */
	par_wrapper -> this_machine = calloc(1, sizeof(struct machine));
	if (par_wrapper -> this_machine == (machine *)NULL)
	{
		print(PRNT_ERR, "Unable to allocate space for this machine\n");
		return 1;
	}

	/* Fill in the default values for port ranges */
	par_wrapper -> low_port = 51000;
	par_wrapper -> high_port = 61000;
	/* Fill in other default values */
	par_wrapper -> this_machine -> rank = -1;
	par_wrapper -> num_procs = -1; 
	par_wrapper -> command_socket = -1;
	par_wrapper -> mpi_executable = strdup("mpiexec.hydra");
	/* Default mutex state */
	pthread_mutex_init(&par_wrapper -> mutex, NULL);

	/* Parse environment variables and command line arguments */
	parse_environment_vars(par_wrapper);
	parse_args(argc, argv, par_wrapper);

	/* Check that required values are filled in */
	if (par_wrapper -> this_machine -> rank < 0)
	{
		print(PRNT_ERR, "Invalid rank (%d). Environment variable or command option not set.\n", 
			par_wrapper -> this_machine -> rank);
		return 2;
	}
	if (par_wrapper -> num_procs < 0)
	{
		print(PRNT_ERR, "Invalid number of processors (%d). Environment variable or command option not set.\n",
			par_wrapper -> num_procs);
		return 2;
	}

	/* Get the IP address for this machine */
	par_wrapper -> this_machine -> ip_addr = get_ip_addr();
	debug(PRNT_INFO, "IP Addr: %s\n", par_wrapper -> this_machine -> ip_addr);
	if (par_wrapper -> this_machine -> ip_addr == (char *)NULL)
	{
		print(PRNT_ERR, "Unable to get the IP address for this machine\n");
		return 2;
	}
	
	/* Get a command port for this machine */
	RC = get_bound_dgram_socket_by_range(par_wrapper -> low_port, 
		par_wrapper -> high_port, &par_wrapper -> this_machine -> port, 
		&par_wrapper -> command_socket);		
	if (RC != 0)
	{
		print(PRNT_ERR, "Unable to bind to command socket\n");
		return 2;
	}
	else
	{
		debug(PRNT_INFO, "Bound to command port: %d\n", par_wrapper -> this_machine -> port);
	}

	/** 
	 * If this is rank 0, point rank 0 to this_machine, otherwise allocate
	 * a new structure for the master
	 */
	if (par_wrapper -> this_machine -> rank == MASTER)
	{
		par_wrapper -> master = par_wrapper -> this_machine;
	}
	else
	{
		par_wrapper -> master = (machine *)calloc(1, sizeof(struct machine));
		if (par_wrapper -> master == (machine *)NULL)
		{
			print(PRNT_ERR, "Unable to allocate space for master\n");
			return 2;
		}
		par_wrapper -> master -> rank = MASTER;
	}

	/* Gather the necessary chirp information */
	RC = chirp_info(par_wrapper);
	if (RC != 0)
	{
		print(PRNT_ERR, "Failure sending/recieving chirp information\n");
		return 2;
	}

	/* Create the listener */
	pthread_create(&par_wrapper -> listener, &attr, &udp_server, (void *)par_wrapper);

	pthread_join(par_wrapper -> listener, NULL);

	return 0;
}
