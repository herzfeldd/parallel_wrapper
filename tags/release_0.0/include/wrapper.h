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
#include "sll.h"
#include "network_util.h"
#include "log.h"

#define MASTER (0u)
#define LOW_PORT (51000u)
#define HIGH_PORT (61000u)
#define TIMEOUT (60*5) /* keep-alive timeout 5 minutes */ 
#define KA_INTERVAL (30) /* keep-alive interval seconds */

extern int exit_flag;

typedef struct machine
{
	uint16_t port; /**< Command Port */
	int rank; /**< Rank [0, N-1] */
	int cpus; /**< The number of CPUs for this rank */
	int unique; /**< Flag noting if this a unique host */
	char *iwd; /**< Initial working directory */
	char *ip_addr; /**< The IP address associated with the machine */
	char *user; /**< The username associated with this machine */
	struct timeval last_alive;
} machine;

typedef struct parallel_wrapper
{
	int cluster_id; /**< The condor cluster id */
	int executable_length; /**< The length of the executable array */
	int num_procs; /**< The number of processors */
	int command_socket; /**< The FD for the command socket */
	int timeout; /**< The keepalive timeout */
	int ka_interval; /**< The keepalive interval */
	uint16_t low_port; /**< The lower port */
	uint16_t high_port; /**< High port */
	machine *this_machine; /**< This machine */
	machine *master; /**< The master machine */
	pthread_t listener; /**< The pthread associated with the network listener */
	pthread_mutex_t mutex; /**< Semaphore */
	char *scratch_dir; /**< The scratch directory to use */
	char *shared_fs; /**< The shared file system */
	char **executable; /**< Array holding the passed executable and args */
	machine **machines; /**< All machines (for the master only) */
	sl_list *symlinks; /**< List of symlinks */
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

extern int chirp_info(parallel_wrapper *par_wrapper);

extern void handle_exit_signal(int signal);

extern void cleanup(parallel_wrapper *par_wrapper, int return_code);

extern void set_environment_vars(parallel_wrapper *par_wrapper);

extern char * get_exec_error_msg(int RC, char *filename);
#endif /* WRAPPER_H */
