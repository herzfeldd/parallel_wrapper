#include "scratch.h"
#include "string_util.h"

#include <sys/types.h>
#include <sys/stat.h>

static int remove_file(char *filename);

/**
 * Create a new scratch directory
 *
 * Attempts to create a new scratch directory. If an existing scratch directory
 * exists for this run, it is first cleaned up.
 *
 * @param par_wrapper The parallel wrapper
 * @return 0 on success otherwise failure
 */
int create_scratch(parallel_wrapper *par_wrapper)
{
	/* Lock the parallel wrapper */
	pthread_mutex_lock(&par_wrapper -> mutex);
	if (par_wrapper -> scratch_dir != (char *)NULL)
	{
		cleanup_scratch(par_wrapper -> scratch_dir);
		free(par_wrapper -> scratch_dir);
	}	
	
	/* Determine the name of theb directory */
	/* Look for TMP variable */
	int free_tmpdir = 0;
	char *tmpdir = getenv("TMP");
	if (tmpdir == (char *)NULL)
	{
		tmpdir = getenv("TMPDIR");
	}
	if (tmpdir == (char *)NULL)
	{
		tmpdir = getenv("TEMPDIR");
	}
	if (tmpdir == (char *)NULL)
	{
		tmpdir = getenv("_CONDOR_SCRATCH_DIR");
	}
	if (tmpdir == (char *)NULL)
	{
		free_tmpdir = 1;
		tmpdir = strdup("/tmp"); /* Last resort */	
	}
	par_wrapper -> scratch_dir = join_paths(tmpdir, ".condor_scratch_XXXXXX");
   	if (par_wrapper -> scratch_dir == (char *)NULL)
	{
		print(PRNT_WARN, "Unable to allocate space for path\n");
		if (free_tmpdir)
		{
			free(tmpdir);
		}
		pthread_mutex_unlock(&par_wrapper -> mutex);
		return 1;
	}	
	par_wrapper -> scratch_dir = mkdtemp(par_wrapper -> scratch_dir);
	if (par_wrapper -> scratch_dir == (char *)NULL)
	{
		print(PRNT_WARN, "Unable to obtain a valid scratch directory\n");
		if (free_tmpdir)
		{
			free(tmpdir);
		}
		pthread_mutex_unlock(&par_wrapper -> mutex);
		return 2;
	}
	debug(PRNT_INFO, "Using scratch directory at %s\n", par_wrapper -> scratch_dir);
	if (free_tmpdir)
	{
		free(tmpdir);
	}
	pthread_mutex_unlock(&par_wrapper -> mutex);

	return 0;
}

/**
 * Attempt to create the machines file
 *
 * This function attempts to create a new machine file in the scratch
 * directory. Machine files are allowed to be partially filled (if the
 * machines have not yet registered).
 *
 * @param The parallel wrapper structure
 * @return 0 on success, otherwise failure
 */
int create_machine_file(parallel_wrapper *par_wrapper)
{
	int RC, i; 
	/* No ranks but zero create a machine file */
	if (par_wrapper -> this_machine -> rank != MASTER)
	{
		print(PRNT_WARN, "Slaves do not create machine files\n");
		return 2;
	}
	if (par_wrapper -> machines == (machine **)NULL)
	{
		print(PRNT_WARN, "Machines list not initialized\n");
		return 3;
	}

	/* Check to make sure that we have a scratch directory */
	if (par_wrapper -> scratch_dir == (char *)NULL)
	{
		RC = create_scratch(par_wrapper);
		if (RC != 0)
		{
			print(PRNT_WARN, "Unable to create machine file - cannot create scratch\n");
			return 1;
		}
	}
	
	char *machine_file_name = join_paths(par_wrapper -> scratch_dir, MACHINE_FILE);
	FILE *fp = fopen(machine_file_name, "w");
	if (fp == (FILE *)NULL)
	{
		print(PRNT_WARN, "Unable to open the machine file for writing\n");
		free(machine_file_name);
		return 4;
	}
	/* Write the contents */
	for (i = 0; i < par_wrapper -> num_procs; i++)
	{
		if (par_wrapper -> machines[i] != (machine *)NULL)
		{
			fprintf(fp, "%s:%d # %s\n", par_wrapper -> machines[i] -> ip_addr,
					par_wrapper -> machines[i] -> cpus, "TODO HOSTNAME");
		}
	}
	free(machine_file_name);
	fclose(fp);
	return 0; /* Success */
}

/**
 * Attempts to construct the ssh wrapper script in the scratch dir
 *
 * Attempts to create a wrapper around the SSH in the scratch
 * directory. This wrapper is used by the hydra executable to spawn
 * the jobs without prompts for unknown hosts.
 *
 * @param scratch_dir the location of the scratch directory
 * @return 0 if success, otherwise failure
 */
int create_ssh_wrapper(char *scratch_dir)
{
	if (scratch_dir == (char *)NULL)
	{
		print(PRNT_WARN, "Unable to create an SSH wrapper in null scratch_dir\n");
		return 1;
	}
	char *ssh_wrapper = join_paths(scratch_dir, SSH_WRAPPER);
	char *ssh_config = join_paths(scratch_dir, SSH_CONFIG);

	FILE *fp = fopen(ssh_wrapper, "w");
	fprintf(fp, "#!/bin/sh\n");
	fprintf(fp, "%s -F %s $@\n", SSH_EXECUTABLE, ssh_config);
	fclose(fp);

	/* Attempt to change the permissions to allow execution */
	if (chmod(ssh_wrapper, S_IRWXU | S_IRGRP | S_IXGRP | S_IWGRP) != 0)
	{
		print(PRNT_WARN, "Unable to set executable permissions on SSH wrapper\n");
	}
	free(ssh_config);
	free(ssh_wrapper);
	return 0;
	
}

/**
 * Attempts to construct the SSH config file in the scratch directory
 *
 * Attempts to create the SSH config file in the scratch directory. The
 * SSH config file is only needed by the master following registration of
 * all hosts. The config file is pointed to by the ssh_wrapper.
 *
 * @param par_wrapper the parallel wrapper structure
 * @return 0 if successful, otherwise error
 */ 
int create_ssh_config(parallel_wrapper *par_wrapper)
{
	int RC, i;
	if (par_wrapper -> this_machine -> rank != MASTER)
	{
		print(PRNT_WARN, "Slaves do not create an SSH config file\n");
		return 1;
	}
	if (par_wrapper -> machines == (machine **)NULL)
	{
		print(PRNT_WARN, "Machines list not initialized\n");
		return 2;
	}

	/* Check to make sure that we have a scratch directory */
	if (par_wrapper -> scratch_dir == (char *)NULL)
	{
		RC = create_scratch(par_wrapper);
		if (RC != 0)
		{
			print(PRNT_WARN, "Unable to create ssh config file - cannot create scratch\n");
			return 2;
		}
	}

	char *ssh_config = join_paths(par_wrapper -> scratch_dir, SSH_CONFIG);
	FILE *fp = fopen(ssh_config, "w");
	if (fp == (FILE *)NULL)
	{
		print(PRNT_WARN, "Unable to open %s for writing\n", ssh_config);
		free(ssh_config);
		return 3;
	}	
	fprintf(fp, "BatchMode=yes\n");
	fprintf(fp, "UserKnownHostsFile=/dev/null\n");
	fprintf(fp, "StrictHostKeyChecking=no\n");
	fprintf(fp, "Cipher=none\n");
	fprintf(fp, "Compression=no\n");
	fprintf(fp, "ForwardX11=no\n");
	fprintf(fp, "FallBackToRsh=yes\n");
	fprintf(fp, "KeepAlive=yes\n");
	fprintf(fp, "ServerAliveInterval=120\n");
	for (i = 0; i < par_wrapper -> num_procs; i++)
	{
		if (par_wrapper -> machines[i] == (machine *)NULL)
		{
			continue; /* Move on to next host */
		}
		fprintf(fp, "Host=%s\n", par_wrapper -> machines[i] -> ip_addr);
		fprintf(fp, "\tUser=%s\n", par_wrapper -> machines[i] -> user);
		fprintf(fp, "\tPort=22\n");
		//fprintf(fp, "\tIdentityFile=%s\n", );
	}	
		
	fclose(fp);

	free(ssh_config);
	/* Write the wrapper */
	create_ssh_wrapper(par_wrapper -> scratch_dir);
	return 0;
}

/**
 * Attempt to clean up the contents of a scratch directory.
 *
 * Attempts to clean up the contents of the a scratch directory.
 * We attempt to remove all of the temporary files and then remove
 * the directory. This function should be called immediately before
 * the entire script exists (or before creating a new scratch directory)
 *
 * @param The location fo the scratch directory to cleanup
 * @return 0 on success otherwise failure
 */
int cleanup_scratch(const char *scratch)
{
	char *temp = NULL;
	struct stat;
	if (scratch == (char *)NULL)
	{
		print(PRNT_WARN, "Cannot cleanup null scratch directory\n");
		return 1;
	}	
	temp = join_paths(scratch, SSHD_LOG);
	remove_file(temp);
	if (temp != (char *)NULL)
	{
		free(temp);
	}
	temp = join_paths(scratch, SSH_CONFIG);
	remove_file(temp);
	if (temp != (char *)NULL)
	{
		free(temp);
	}
	temp = join_paths(scratch, SSH_WRAPPER);
	remove_file(temp);
	if (temp != (char *)NULL)
	{
		free(temp);
	}
	temp = join_paths(scratch, MACHINE_FILE);
	remove_file(temp);
	if (temp != (char *)NULL)
	{
		free(temp);
	}
	/* Now attempt to remove the directory */
	errno = 0;
	rmdir(scratch);
	if (errno != 0)
	{
		print(PRNT_WARN, "Unable to remove scratch directory %s\n", scratch);
		return 2;
	}
	debug(PRNT_INFO, "Successfully removed scratch directory %s\n", scratch);
	return 0; /* Success */
}

/**
 * Attempts to remove the file located at filename
 * 
 * We only attempt to remove the file if it exists. If it
 * does not exist - we return 0. If the file does exist and
 * we are unable to remove it - we print an error and return
 * a non-zero return code.
 *
 * @param filename The file to remove
 * @return 0 on SUCCESS, otherwise failure
 */
static int remove_file(char *filename)
{
	int RC;
	struct stat st;
	if (filename == (char *)NULL)
	{
		return 0; /* Nothing to do */
	}
	if (stat(filename, &st) != 0)
	{
		/* The file does not exist */
		return 0;
	}
	/* The file does exist, attempt a remove */
	errno = 0;
	remove(filename);
	RC = errno;
	if (RC != 0)
	{
		print(PRNT_WARN, "Unable to remove %s\n", filename);
	}
	return RC;
}

