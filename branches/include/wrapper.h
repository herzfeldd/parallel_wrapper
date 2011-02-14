/**
 * @file The main wrapper header file
 */

#ifndef WRAPPER_H
#define WRAPPER_H

#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "udp.h"
#include "network_util.h"
#include "log.h"

#define MASTER (0u)

typedef struct machine
{
	uint16_t port; /**< Command Port */
	int rank; /**< Rank [0, N-1] */
	int cpus; /**< The number of CPUs for this rank */
	int unique; /**< Flag noting if this a unique host */
	char *iwd; /**< Initial working directory */
	char *ip_addr; /**< The IP address associated with the machine */
	struct timeval last_alive;
} machine;

typedef struct parallel_wrapper
{
	int executable_length; /**< The length of the executable array */
	int num_procs; /**< The number of processors */
	int command_socket; /**< The FD for the command socket */
	uint16_t low_port; /**< The lower port */
	uint16_t high_port; /**< High port */
	machine *this_machine; /**< This machine */
	machine *master; /**< The master machine */
	pthread_t listener; /**< The pthread associated with the network listener */
	pthread_mutex_t mutex; /**< Semaphore */
	char *mpi_flags; /**< Flags to the MPI executable */
	char *mpi_executable; /**< MPI Executable */
	char **executable; /**< Array holding the passed executable and args */
} parallel_wrapper;

/**
 * Help functionality
 */
extern void help(void);

/**
 * Parse command line args
 */
extern int parse_args(int argc, char **argv, parallel_wrapper *par_wrapper);

/**
 * Parse environment variables
 */
extern void parse_environment_vars(parallel_wrapper *par_wrapper);

extern void default_pthead_attr(pthread_attr_t *attr);
#endif /* WRAPPER_H */
