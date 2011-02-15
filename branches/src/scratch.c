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
	par_wrapper -> scratch_dir = mkdtemp("condor_scratch_XXXXXX");
	if (par_wrapper -> scratch_dir == (char *)NULL)
	{
		print(PRNT_WARN, "Unable to obtain a valid scratch directory\n");
		pthread_mutex_unlock(&par_wrapper -> mutex);
		return 1;
	}
	debug(PRNT_INFO, "Using scratch directory at %s\n", par_wrapper -> scratch_dir);

	pthread_mutex_unlock(&par_wrapper -> mutex);
	return 0;
}

/**
 * Attempt to create the machines file
 */


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
