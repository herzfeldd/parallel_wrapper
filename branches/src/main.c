
#include "wrapper.h"
#include "chirp_util.h"

int main(int argc, char **argv)
{
	int RC;
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
	
	return 0;
}
