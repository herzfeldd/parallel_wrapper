#ifndef PARALLEL_H
#define PARALLEL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include "chirp_client.h"

#define BUFFER_SIZE (1024u)

typedef struct PROC
{
	char *sinful; /**< The sinful string for this processor */
	int rank; /**< the rank of this processor */
	int cpus; /**< The number of cpus assigned to this host */
	uint16_t command_port;
	time_t last_update; /**< The last update time for this host */
	int not_unique; /**< Unique host */
} PROC;

/**
 * Filled when ACK is recieved
 */
typedef struct ACK
{
	int received;
	int rank;
} ACK; 

typedef struct PARALLEL_WRAPPER
{
	unsigned int low_port;
	unsigned int high_port;
	uint16_t command_port; /**< The assigned command port */
	int command_socket; /**< The command socket */
	pthread_mutex_t mutex; /**< Structure semaphore */
	int num_procs;
	int rank;
	char *rank0_sinful; /**< The sinful string */
	char *mpi_flags; /**< Global flags to MPI executable */
	char *mpi_executable; /**< MPI Executable (path) */
	char *scratch_dir; /**< Scratch directory (path) */
	char **executable; /**< Array holding the passed executable and args */
   	int executable_length;	
	struct chirp_client *chirp; /**< Chirp connection */
	struct PROC **processors;
	int registered_processors;
	struct ACK *ack;
} PARALLEL_WRAPPER;

typedef struct MESSAGE
{
	struct PARALLEL_WRAPPER *par_wrapper;
	char *buffer;
	int buffer_length;
	struct sockaddr_storage from;
	socklen_t from_len;
} MESSAGE;

/**
 * Help functionality
 */
extern void help(void);

/**
 * Parse command line args
 */
extern int parse_args(int argc, char **argv, PARALLEL_WRAPPER *par_wrapper);

/**
 * Parse environment variables
 */
extern void parse_environment_vars(PARALLEL_WRAPPER *par_wrapper);


void cleanup(int RC, PARALLEL_WRAPPER *);
#endif /* PARALLEL_H */
