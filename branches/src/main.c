#include "wrapper.h"
#include "chirp_util.h"
#include "scratch.h"
#include <signal.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <pwd.h>

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
	par_wrapper -> low_port = LOW_PORT;
	par_wrapper -> high_port = HIGH_PORT;
	/* Fill in other default values */
	par_wrapper -> this_machine -> rank = -1;
	par_wrapper -> num_procs = -1; 
	par_wrapper -> command_socket = -1;
	par_wrapper -> ka_interval = KA_INTERVAL;
	par_wrapper -> timeout = TIMEOUT;
	/* Default mutex state */
	pthread_mutex_init(&par_wrapper -> mutex, NULL);
	/* Allocate a list of symlinks */
	par_wrapper -> symlinks = sll_get_list();
	/* Get the initial working directory */
	par_wrapper -> this_machine -> iwd = getcwd(NULL, 0); /* Allocates space */
	/* Determine our name */
	uid_t uid = getuid();
	struct passwd *user_stats = getpwuid(uid);
	if (user_stats == (struct passwd *)NULL)
	{
		print(PRNT_WARN, "Unable to determine the name of the current user - assuming 'nobody'\n");
		par_wrapper -> this_machine -> user = strdup("nobody");
	}
	else
	{
		par_wrapper -> this_machine -> user = strdup(user_stats -> pw_name);	

	}

	/* Parse environment variables and command line arguments */
	parse_environment_vars(par_wrapper);
	parse_args(argc, argv, par_wrapper);

	/* Set the environment variables */
	set_environment_vars(par_wrapper);

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
	if (par_wrapper -> timeout <= 2*par_wrapper -> ka_interval)
	{
		print(PRNT_WARN, "Keep-alive interval and timeout too close. Using default values.\n");
		par_wrapper -> timeout = TIMEOUT;
		par_wrapper -> ka_interval = KA_INTERVAL;
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
	
	/* Create the scratch directory */
	create_scratch(par_wrapper);

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
		int i, j;
		for (i = 0; i < par_wrapper -> num_procs; i++)
		{
			while ( 1 )
			{
				if (par_wrapper -> machines[i] != NULL)
				{
					break;
				}
				sleep(1);
				j++;
				if (j == par_wrapper -> timeout)
				{
					print(PRNT_WARN, "Rank %d not registered - giving up on it\n", i);
					break;
				}
				if ((j % par_wrapper -> ka_interval) == 0)
				{
					debug(PRNT_INFO, "Waiting for registration from rank %d\n", i);
				}
			}
		}
		debug(PRNT_INFO, "Finished machine registration.\n", par_wrapper -> num_procs);
		/* Create the machines file */
		RC = create_machine_file(par_wrapper);
		if (RC != 0)
		{
			print(PRNT_ERR, "Unable to create the machines files");
			cleanup(par_wrapper, 5);
		}
		RC = create_ssh_config(par_wrapper);
		if (RC != 0)
		{
			print(PRNT_ERR, "Unable to create the machines files");
			cleanup(par_wrapper, 5);
		}
	}
	else /* Register with the master */
	{
		struct timeval old_time = par_wrapper -> master -> last_alive;
		while ( 1 )
		{
			RC = register_cmd(par_wrapper -> command_socket, par_wrapper -> this_machine -> rank,
				par_wrapper -> this_machine -> cpus,
				par_wrapper -> this_machine -> iwd, par_wrapper -> this_machine -> user, par_wrapper -> master -> ip_addr, 
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
			if (par_wrapper -> machines[i] == (machine *)NULL)
			{
				continue;
			}
			/* All machines are initially unique */
			par_wrapper -> machines[i] -> unique = 1; 
		}
		for (i = 0; i < par_wrapper -> num_procs; i++)
		{
			if (par_wrapper -> machines[i] == (machine *)NULL)
			{
				continue;
			}
			if (par_wrapper -> machines[i] -> unique == 0)
			{
				continue; /* This host is already not unique */
			}
			for (j = i + 1; j < par_wrapper -> num_procs; j++)
			{
				if (par_wrapper -> machines[j] == (machine *)NULL)
				{
					continue;
				}
				if (par_wrapper -> machines[j] -> unique == 0)
				{
					continue; /* This host is already not unique */
				}	
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
			if (par_wrapper -> machines[i] == (machine *)NULL)
			{
				continue;
			}
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
				if (par_wrapper -> machines[i] == (machine *)NULL)
				{
					continue;
				}
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

	/* Start up the MPI executable */
	if (par_wrapper -> this_machine -> rank == MASTER)
	{
		pid_t p = fork();
		if (p == (pid_t) -1)
		{
			print(PRNT_ERR, "Fork failed\n");
			cleanup(par_wrapper, 10);
		}
		else if (p == (pid_t) 0)
		{
			/* I am the child */
			prctl(PR_SET_PDEATHSIG, SIGTERM);
			/* Set environment variables */
			char *machine_file = join_paths(par_wrapper -> scratch_dir, MACHINE_FILE);
			char *ssh_config = join_paths(par_wrapper -> scratch_dir, SSH_CONFIG);
			setenv("MACHINE_FILE", machine_file, 1); 
			setenv("SSH_CONFIG", ssh_config, 1);
		   	free(machine_file);
			free(ssh_config);	
			char *ssh_wrapper = join_paths(par_wrapper -> scratch_dir, SSH_WRAPPER);
			setenv("SSH_WRAPPER", ssh_wrapper, 1);
			free(ssh_wrapper);
			char temp_str[1024];
			
			snprintf(temp_str, 1024, "%d", par_wrapper -> num_procs);
			setenv("NUM_MACHINES", temp_str, 1);
		
			/* Determine the total number of cpus allocated to this task */
			int total_cpus = 0;
			int i;
			for (i = 0; i < par_wrapper -> num_procs; i++)
			{
				if (par_wrapper -> machines[i] == (machine *)NULL)
				{
					continue;
				}
				total_cpus += par_wrapper -> machines[i] -> cpus;
			}
			snprintf(temp_str, 1024, "%d", total_cpus);
			setenv("CPUS", temp_str, 1);
			setenv("NUM_PROCS", temp_str, 1);

			snprintf(temp_str, 1024, "%d", par_wrapper -> this_machine -> rank);
			setenv("RANK", temp_str, 1);
			
			snprintf(temp_str, 1024, "%d", par_wrapper -> cluster_id);
			setenv("CLUSTER_ID", temp_str, 1);
			
			snprintf(temp_str, 1024, "%d", par_wrapper -> this_machine -> port);
			setenv("CMD_PORT", temp_str, 1);
			
			snprintf(temp_str, 1024, "%d", par_wrapper -> this_machine -> cpus);
			setenv("REQUEST_CPUS", temp_str, 1);

			setenv("SCRATCH_DIR", par_wrapper -> scratch_dir, 1);
			setenv("IWD", par_wrapper -> this_machine -> iwd, 1);
			setenv("IP_ADDR", par_wrapper -> this_machine -> ip_addr, 1);
			setenv("TRANSFER_FILES", shared_fs != 0 ? "TRUE" : "FALSE", 1); 
			setenv("SHARED_FS", shared_fs != 0 ? par_wrapper -> this_machine -> iwd : par_wrapper -> shared_fs, 1);
			setenv("SHARED_DIR", shared_fs != 0 ? par_wrapper -> this_machine -> iwd : par_wrapper -> shared_fs, 1);
			/* TODO: SSH ENVS */

			/* Search in path */
			int process_RC = execvp(par_wrapper -> executable[0], &par_wrapper -> executable[0]);
			if (process_RC != 0)
			{
				print(PRNT_ERR, "%s\n", get_exec_error_msg(errno, par_wrapper -> executable[0]));
			}
			exit(process_RC);	
		}
		else
		{
			/* I am the parent */
			int child_status = 0;
			waitpid(p, &child_status, WUNTRACED); /* Wait for the child to finish */
			cleanup(par_wrapper, child_status);
		}

	}

	/* Always wait for the listener */
	pthread_join(par_wrapper -> listener, NULL);

	return 0;
}
