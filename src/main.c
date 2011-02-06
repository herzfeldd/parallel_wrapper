#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "wrapper.h"
#include "log.h"
#include "commands.h"
#include "chirp_client.h" 

/**
 * Main.c
 */
int main(int argc, char **argv)
{
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setstacksize(&thread_attr, BUFFER_SIZE *BUFFER_SIZE);
	PARALLEL_WRAPPER *par_wrapper = calloc(1, sizeof(struct PARALLEL_WRAPPER));
	/* Set low range and high range */
	par_wrapper -> low_port = 1024;
	par_wrapper -> high_port = 2048;
	par_wrapper -> num_procs = -1;
	par_wrapper -> rank = -1;
	/* Fill in the default value of the mpi executable */
	par_wrapper -> mpi_executable = strdup("mpiexec.hydra");
	//par_wrapper -> scratch = mkdtemp(".scratch_XXXXXXXX");
	pthread_mutex_init(&par_wrapper -> mutex, NULL);

	/* Open the command port */
	pthread_mutex_lock(&par_wrapper -> mutex);
	port_by_range(par_wrapper, &par_wrapper -> command_port, &par_wrapper -> command_socket);
	par_wrapper -> rank0_sinful = get_sinful_string(par_wrapper -> command_port);
	debug(PRNT_INFO, "Rank 0 sinful string: %s\n", par_wrapper -> rank0_sinful);
	pthread_mutex_unlock(&par_wrapper -> mutex);

	/* Spawn a thread to service the network */
	pthread_t thread;
	pthread_create(&thread, &thread_attr, &service_network, par_wrapper);

	/* Check for valid environment variables */
	pthread_mutex_lock(&par_wrapper -> mutex);
	parse_environment_vars(par_wrapper);
	pthread_mutex_unlock(&par_wrapper -> mutex);

	/* Parse arguments */
	pthread_mutex_lock(&par_wrapper -> mutex);
	parse_args(argc, argv, par_wrapper);
	pthread_mutex_unlock(&par_wrapper -> mutex);

	/* Check the structure */
	if (par_wrapper -> num_procs <= 0)
	{
		print(PRNT_ERR, "The value of num_procs is invalid. Consider setting _CONDOR_NUMPROCS\n");
		return 1;
	}
	if (par_wrapper -> rank < 0)
	{
		print(PRNT_ERR, "The rank is invalid. Consider setting _CONDOR_PROCNO\n");
		return 2;
	}


	/* Attempt to open a chirp context */
	struct chirp_client *chirp = chirp_client_connect_default();
	if (chirp == (struct chirp_client *)NULL)
	{
		print(PRNT_ERR, "Unable to create chirp connnection\n");
		return 3;
	}

	/* Error checking passed */
	if (par_wrapper -> rank == 0)
	{
		/* Wait for rank 0 sinful string */
		printf("Sinful = %s\n", par_wrapper -> rank0_sinful);
		/* I am rank 0 */
		int RC = chirp_client_set_job_attr(chirp, "RANK0_SINFUL", par_wrapper -> rank0_sinful);	
		if (RC != 0)
		{
			print(PRNT_ERR, "Unable to set RANK0_SINFUL h\n");
			exit(5);
		}
	}
	else
	{
		char *sinful_string = NULL;
		/* I am not rank 0 */
		while ( 1 )
		{
			int RC = chirp_client_get_job_attr(chirp, "RANK0_SINFUL", &sinful_string);
			if (RC == 0)
			{
				break;
			}
			sleep(1); /* Sleep for 1 second */
			print(PRNT_WARN, "Unable to get sinful string\n");
		}
		print(PRNT_INFO, "Got sinful string: %s\n", sinful_string);
	}
	chirp_client_disconnect(chirp);


	void *RC;
	pthread_join(thread, &RC);
	return 0;
}


