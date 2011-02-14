
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

	/* Fill in the default values for port ranges */
	par_wrapper -> low_port = 51000;
	par_wrapper -> high_port = 61000;

	/* Create structures for this machine */
	par_wrapper -> this_machine = calloc(1, sizeof(struct machine));
	if (par_wrapper -> this_machine == (machine *)NULL)
	{
		print(PRNT_ERR, "Unable to allocate space for this machine\n");
		return 1;
	}
	
	/* Parse environment variables and command line arguments */
	parse_environment_vars(par_wrapper);
	parse_args(argc, argv, par_wrapper);

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
		if (par_wrapper -> master)
		{
			print(PRNT_ERR, "Unable to allocate space for master\n");
			return 2;
		}
	}

	struct chirp_client *chirp = chirp_client_connect_default();
	if (chirp == (struct chirp_client *)NULL)
	{
		print(PRNT_ERR, "Unable to open chirp context\n");
		return 1;
	}

	RC = get_chirp_integer(chirp, "RequestCpus", &par_wrapper -> this_machine -> cpus);
	if (RC != 0)
	{
		print(PRNT_WARN, "Failed to get RequestCpus, assuming 1\n");
		par_wrapper -> this_machine -> cpus = 1;
	}

	/* Get the initial working directory */
	RC = get_chirp_string(chirp, "Iwd", &par_wrapper -> this_machine -> iwd);
	if (RC != 0)
	{
		par_wrapper -> this_machine -> iwd = getcwd(NULL, 0); /* Allocates space */
		print(PRNT_WARN, "Failed to get Iwd, assuming current directory: %s\n",
				par_wrapper -> this_machine -> iwd);
	}	

	/* Create the listener */
	pthread_create(&par_wrapper -> listener, &attr, &udp_server, (void *)par_wrapper);

	pthread_join(par_wrapper -> listener, NULL);

	return 0;
}
