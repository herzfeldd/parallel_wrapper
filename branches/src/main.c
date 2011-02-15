
#include "wrapper.h"
#include "chirp_util.h"
#include <signal.h>

int main(int argc, char **argv)
{
	int RC;
	pthread_attr_t attr;
	default_pthead_attr(&attr);

	/* Install command signal handlers */
	signal(SIGINT, handle_exit_signal);
	signal(SIGTERM, handle_exit_signal);
	signal(SIGHUP, handle_exit_signal);


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
	/* Allocate a list of symlinks */
	par_wrapper -> symlinks = sll_get_list();
	/* Get the initial working directory */
	par_wrapper -> this_machine -> iwd = getcwd(NULL, 0); /* Allocates space */

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
		par_wrapper -> machines = (machine **) calloc(par_wrapper -> num_procs, sizeof(machine *));
		if (par_wrapper -> machines == (machine **)NULL)
		{
			print(PRNT_ERR, "Unable to allocate space for machines array\n");
			return 3;
		}
		par_wrapper -> machines[0] = par_wrapper -> master;
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

	/* If I am the MASTER, wait for all ranks to register */
	if (par_wrapper -> this_machine -> rank == MASTER)
	{
		int i;
		for (i = 0; i < par_wrapper -> num_procs; i++)
		{
			while ( 1 )
			{
				if (par_wrapper -> machines[i] != NULL)
				{
					break;
				}
				sleep(1);
			}
		}
		debug(PRNT_INFO, "All machines (%d) registers\n", par_wrapper -> num_procs);

	}
	else /* Register with the master */
	{
		struct timeval old_time = par_wrapper -> master -> last_alive;
		while ( 1 )
		{
			RC = register_cmd(par_wrapper -> command_socket, par_wrapper -> this_machine -> rank,
				par_wrapper -> this_machine -> iwd, par_wrapper -> master -> ip_addr, 
				par_wrapper -> master -> port);		
			if (RC == 0 && ! timercmp(&old_time, &par_wrapper -> master -> last_alive, =))
			{
				break;
			}		
			sleep(1);
		}
	}

	/* MASTER - Identify unique hosts */
	if (par_wrapper -> this_machine -> rank == MASTER)
	{
		int i, j;
		for (i = 0; i < par_wrapper -> num_procs; i++)
		{
			/* All machines are initially unique */
			par_wrapper -> machines[i] -> unique = 1; 
		}
		for (i = 0; i < par_wrapper -> num_procs; i++)
		{
			for (j = i + 1; j < par_wrapper -> num_procs; j++)
			{
				if (strcmp(par_wrapper -> machines[i] -> ip_addr, par_wrapper -> machines[j] -> ip_addr) == 0)
				{
					par_wrapper -> machines[j] -> unique = 0;
					debug(PRNT_INFO, "Rank %d (%s:%d) is not unique (some host as %d).\n", 
							j, par_wrapper -> machines[j] -> ip_addr, 
							par_wrapper -> machines[j] -> port, i);
				}	
			}
		}
	}
	
	int shared_fs = 1; /* Flag which denotes a shared fs */

	/* If I am rank 0 - determine if we need to create a fake file system */
	if (par_wrapper -> this_machine -> rank == MASTER)
	{
		int i;
		for (i = 0; i < par_wrapper -> num_procs; i++)
		{
			if (strcmp(par_wrapper -> machines[i] -> iwd, par_wrapper -> master -> iwd) != 0)
			{
				shared_fs = 0;
			}
		}
		if (shared_fs == 0)
		{
			char fake_fs[1024];
			time_t curr_time = time(NULL);
			snprintf(fake_fs, 1024, "/tmp/condor_hydra_%d_%ld", par_wrapper -> cluster_id, 
					(long)curr_time);
			debug(PRNT_INFO, "Using fake file system (%s). IWD's across ranks differ\n", fake_fs);
			for (i = 0; i < par_wrapper -> num_procs; i++)
			{
				if (! par_wrapper -> machines[i] -> unique)
				{
					continue;
				}
				/* Send the command to create softlinks */
				create_link(par_wrapper -> command_socket, par_wrapper -> machines[i] -> iwd,
						fake_fs, par_wrapper -> machines[i] -> ip_addr, 
						par_wrapper -> machines[i] -> port);
			}
			par_wrapper -> shared_fs = strdup(fake_fs);
		}
		else 
		{
			par_wrapper -> shared_fs = strdup(par_wrapper -> master -> iwd);
			debug(PRNT_INFO, "Using a shared file system, IWD = %s\n", par_wrapper -> master -> iwd);
		}
	}

	pthread_join(par_wrapper -> listener, NULL);

	return 0;
}
