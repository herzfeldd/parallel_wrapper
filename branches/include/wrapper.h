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
#include "log.h"

#define MASTER (0u)

typedef struct machine
{
	int rank; /**< Rank [0, N-1] */
	int cpus; /**< The number of CPUs for this rank */
	char *iwd; /**< Initial working directory */
	int port; /**< Command Port */
	char *ip_addr; /**< The IP address associated with the machine */
	struct timeval last_alive;
	int unique; /**< Flag noting if this a unique host */
} machine;

typedef struct parallel_wrapper
{
	int num_procs; /**< The number of processors */
	uint16_t low_port; /**< The lower port */
	uint16_t high_port; /**< High port */
	machine *this_machine; /**< This machine */
	machine *master; /**< The master machine */
	pthread_t listener; /**< The pthread associated with the network listener */
	pthread_mutex_t mutex; /**< Semaphore */
	char *mpi_flags; /**< Flags to the MPI executable */
	char *mpi_executable; /**< MPI Executable */
	int executable_length;
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

#endif /* WRAPPER_H */
