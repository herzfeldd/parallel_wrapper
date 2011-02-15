#ifndef SCRATCH_H
#define SCRATCH_H

#include "wrapper.h"

/**
 * Define the names of SSH files/executables
 */ 
#define SSHD_EXECUTABLE "sshd"
#define SSH_EXECUTABLE "ssh"
#define SSH_KEYGEN_EXECUTABLE "ssh-keygen"
#define SSHD_LOG "sshd.log"
#define SSH_CONFIG "ssh_config"
#define SSH_WRAPPER "ssh_wrapper"


/**
 * Define the name of the machines file
 */
#define MACHINE_FILE "machines.txt"


extern int cleanup_scratch(const char *scratch);

extern int create_scratch(parallel_wrapper *par_wrapper);

extern int create_machine_file(parallel_wrapper *par_wrapper);

extern int create_ssh_wrapper(char *scratch_dir);

extern int create_ssh_config(parallel_wrapper *par_wrapper);
#endif /* SCRATCH_H */
