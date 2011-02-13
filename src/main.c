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

static int remove_quotes(char *string);
static int get_distinct_hosts(PARALLEL_WRAPPER *par_wrapper);

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
	par_wrapper -> low_port = 51000;
	par_wrapper -> high_port = 61000;
	par_wrapper -> num_procs = -1;
	par_wrapper -> rank = -1;
	/* Fill in the default value of the mpi executable */
	par_wrapper -> mpi_executable = strdup("mpiexec.hydra");
	/* Allocate the ack structure */
	par_wrapper -> ack = (struct ACK *)calloc(1, sizeof(struct ACK));
	//par_wrapper -> scratch = mkdtemp(".scratch_XXXXXXXX");
	pthread_mutex_init(&par_wrapper -> mutex, NULL);

	/* Open the command port */
	pthread_mutex_lock(&par_wrapper -> mutex);
	port_by_range(par_wrapper, &par_wrapper -> command_port, &par_wrapper -> command_socket);
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
	/* Allocate the processors array if I am rank 0 */
	if (par_wrapper -> rank == 0)
	{
		pthread_mutex_lock(&par_wrapper -> mutex);
		par_wrapper -> processors = (struct PROC **) calloc(par_wrapper -> num_procs, sizeof(struct PROC *)); /* Don't need one for ourselves */
		par_wrapper -> registered_processors++;
		par_wrapper -> processors[0] = (struct PROC *) calloc(1, sizeof(struct PROC));
		par_wrapper -> processors[0] -> sinful = get_hostname();
;
		par_wrapper -> processors[0] -> cpus = 1;
		par_wrapper -> processors[0] -> command_port = par_wrapper -> command_port;
		pthread_mutex_unlock(&par_wrapper -> mutex);
	}
	
	/* Attempt to open a chirp context */
	
	pthread_mutex_lock(&par_wrapper -> mutex);
	par_wrapper -> chirp = chirp_client_connect_default();
	pthread_mutex_unlock(&par_wrapper -> mutex);
	if (par_wrapper -> chirp == (struct chirp_client *)NULL)
	{
		print(PRNT_ERR, "Unable to create chirp connnection\n");
		return 3;
	}

	/* Error checking passed */
	if (par_wrapper -> rank == 0)
	{	
		pthread_mutex_lock(&par_wrapper -> mutex);
		par_wrapper -> rank0_sinful = get_hostname_sinful_string(par_wrapper -> command_port);
		pthread_mutex_unlock(&par_wrapper -> mutex);	
		
		/* Wait for rank 0 sinful string */
		char *temp = malloc(strlen(par_wrapper -> rank0_sinful) + 10);
		sprintf(temp, "\"%s\"", par_wrapper -> rank0_sinful);
		/* I am rank 0 */
		int RC = chirp_client_set_job_attr(par_wrapper -> chirp, "RANK0_SINFUL", temp);	
		if (RC != 0)
		{
			print(PRNT_ERR, "Unable to set RANK0_SINFUL h\n");
			exit(5);
		}
		print(PRNT_INFO, "Successfully set sinful string RANK0_SINFUL=%s\n", temp);
		free(temp);
		
		/* Determine the number of processors */
		int procs = 1;
		char *RequestCpus = NULL;
		char *next;
		int length = chirp_client_get_job_attr(par_wrapper -> chirp, "RequestCpus", &RequestCpus);
		if (length > 0)
		{
			RequestCpus[length] = '\0';
		}
		errno = 0;
		procs = strtol(RequestCpus, &next, 10);
		if (errno != 0 || next == RequestCpus)
		{
			print(PRNT_WARN, "Failed to get RequestCpus = %s\n", RequestCpus);
			procs = 1;
		}

		pthread_mutex_lock(&par_wrapper -> mutex);
		par_wrapper -> processors[0] -> cpus = procs;
		pthread_mutex_unlock(&par_wrapper -> mutex);	

		/* Busy wait for all nodes to register */
		while (par_wrapper -> registered_processors < par_wrapper -> num_procs)
		{
			printf("Registered %d of %d\n", par_wrapper -> registered_processors, par_wrapper -> num_procs);
			usleep(500 * 1000);
		}
		print(PRNT_INFO, "All processors registered\n");
		get_distinct_hosts(par_wrapper);
		int i;
		for(i = 0; i < par_wrapper -> num_procs; i++)
		{
			printf("%s #Rank %d, Procs %d\n", par_wrapper -> processors[i] -> sinful, i, par_wrapper -> processors[i] -> cpus);
		}
		/* Attempt to look up "ShouldTransferFiles" */
		char *ShouldTransferFiles = NULL;
		chirp_client_get_job_attr(par_wrapper -> chirp, "ShouldTransferFiles", &ShouldTransferFiles);
		if (strstr(ShouldTransferFiles, "NO") == NULL && strstr(ShouldTransferFiles, "NEVER") == NULL) 
		{
			print(PRNT_INFO, "TransferFiles Required - creating fake shared file system\n");
		}
		else
		{
			print(PRNT_INFO, "No Transfer Necessary\n");
		}

	}
	else
	{
		char *sinful_string = NULL;
		/* I am not rank 0 */
		int length = 0;
		while ( 1 )
		{
			length = chirp_client_get_job_attr(par_wrapper -> chirp, "RANK0_SINFUL", &sinful_string);
			if (length > 0 && sinful_string != (char *)NULL && strncmp(sinful_string, "\"UNDEFINED\"", 11) != 0 && strncmp(sinful_string, "UNDEFINED", 9) != 0)
			{
				break;
			}
			sleep(1); /* Sleep for 1 second */
			print(PRNT_WARN, "Unable to get sinful string\n");
		}
		/* Convert sinful string into normal string (removing quotes) */
		sinful_string[length] = '\0';
		remove_quotes(sinful_string);
		pthread_mutex_lock(&par_wrapper -> mutex);
		par_wrapper -> rank0_sinful = sinful_string;
		pthread_mutex_unlock(&par_wrapper -> mutex);
		print(PRNT_INFO, "Got sinful string: %s\n", par_wrapper -> rank0_sinful);

		/* Determine the number of processors */
		int procs = 1;
		char *RequestCpus = NULL;
		char *next;
		length = chirp_client_get_job_attr(par_wrapper -> chirp, "RequestCpus", &RequestCpus);
		if (length > 0)
		{
			RequestCpus[length] = '\0';
		}
		errno = 0;
		procs = strtol(RequestCpus, &next, 10);
		if (errno != 0 || next == RequestCpus)
		{
			print(PRNT_WARN, "Failed to get RequestCpus = %s\n", RequestCpus);
			procs = 1;
		}

		/* Register with rank 0 */
		char command[1024];
		snprintf(command, 1024, "%d %d %d %d", CMD_REGISTER, par_wrapper -> rank, par_wrapper -> command_port, procs);
		/* Clear the ack */
		pthread_mutex_lock(&par_wrapper -> mutex);
		par_wrapper -> ack -> received = 0;
		pthread_mutex_unlock(&par_wrapper -> mutex);
		while (par_wrapper -> ack -> received == 0 && par_wrapper -> ack -> rank != par_wrapper -> rank)
		{
			send_command_to_rank(par_wrapper, 0, command);
			sleep(1);
		}
		print(PRNT_INFO, "Successfully registered with rank 0\n");
	}

	void *RC;
	pthread_join(thread, &RC);
	print(PRNT_ERR, "ASSERTION FAILED - EXITTING WITH STATUS 0!\n");
	return 0;
}

/**
 * Remove the quotes from a string
 */
static int remove_quotes(char *string)
{
	if (string == (char *)NULL || strlen(string) == 0)
	{
		return 0;
	}
	int length = strlen(string);
	int i = 0;
	int index = 0;
	while (i < length && index < length)
	{
		if (string[i] == '"' || string[i] == '\'')
		{
			i++;
		}
		if (i < length)
		{
			string[index] = string[i];
		}
		i++;
		index++;
	}
	if (index < length)
	{
		string[index-1] = '\0';
	}
	return 0;
}

/**
 * Set distinct hosts
 */
static int get_distinct_hosts(PARALLEL_WRAPPER *par_wrapper)
{
	int i, j;
	if (par_wrapper -> num_procs <= 1)
	{
		return 0;
	}
	for (i = 0; i < par_wrapper -> num_procs; i++)
	{
		if (par_wrapper -> processors[i] -> not_unique != 0)
		{
			continue; /* This is a non-unique host, no bother comparing */
		}
		for (j = i + 1; j < par_wrapper -> num_procs; j++)
		{
			if (strcmp(par_wrapper -> processors[i] -> sinful, par_wrapper -> processors[j] -> sinful) == 0);
			{
				par_wrapper -> processors[j] -> not_unique = 1;
			}
		}
	}

	return 0;
}
