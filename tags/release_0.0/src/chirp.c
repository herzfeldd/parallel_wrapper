#include "wrapper.h"
#include "chirp_util.h"
#include "chirp_client.h"
/**
 * Sends and receives the necessary chirp information back to the schedd
 * 
 * Attempts to fill in the passed parallel_wrapper structure by 
 * communicating back to the schedd via chirp. The chirp connection
 * is removed after all of the necessary information has been filled in
 */
int chirp_info(parallel_wrapper *par_wrapper)
{
	int RC;
	/* Lock the parallel wrapper structure */
	pthread_mutex_lock(&par_wrapper -> mutex);
	/* Change to the TMP directory */
	char *dir = getenv("TMPDIR");
	char *prev_dir = calloc(1024, sizeof(char));
	getcwd(prev_dir, 1024);
	if (dir == (char *)NULL)
	{
		dir = getenv("_CONDOR_SCRATCH_DIR");
	}
	if (dir != (char *)NULL)
	{
		chdir(dir);
	}
	struct chirp_client *chirp = chirp_client_connect_default();
	if (chirp == (struct chirp_client *)NULL)
	{
		print(PRNT_ERR, "Unable to open chirp context\n");
		return 1;
	}
	/* Change back to the original directory */
	if (dir != (char *)NULL)
	{
		chdir(prev_dir);
	}
	free(prev_dir);
	pthread_mutex_unlock(&par_wrapper -> mutex);

	RC = get_chirp_integer(chirp, "RequestCpus", &par_wrapper -> this_machine -> cpus);
	if (RC != 0)
	{
		print(PRNT_WARN, "Failed to get RequestCpus, assuming 1\n");
		par_wrapper -> this_machine -> cpus = 1;
	}
	
	RC = get_chirp_integer(chirp, "ClusterId", &par_wrapper -> cluster_id);
	if (RC != 0)
	{
		print(PRNT_WARN, "Failed to get ClusterId, assuming random integer\n");
		par_wrapper -> cluster_id = rand();
	}

	/* Send the MASTER information back to the schedd */
	if (par_wrapper -> this_machine -> rank == MASTER)
	{
		char temp_str[1024];
		snprintf(temp_str, 1024, "\"%s\"", par_wrapper -> this_machine -> ip_addr);
		RC = chirp_client_set_job_attr(chirp, "MasterIpAddr",  temp_str);
		if (RC != 0)
		{
			print(PRNT_ERR, "Unable to send MasterIpAddr to the chirp server\n");
			return 3;
		}
		snprintf(temp_str, 1024, "%d", par_wrapper -> this_machine -> port);
		RC = chirp_client_set_job_attr(chirp, "MasterPort", temp_str);
		if (RC != 0)
		{
			print(PRNT_ERR, "Unable to send MasterPort to the chirp server\n");
			return 4;
		}
	}
	else /* I am not the master */
	{
		/* Attempt to get the MasterIpAddr and MasterPort for the chirp server */
		while ( 1 )
		{
			RC = get_chirp_string(chirp, "MasterIpAddr", &par_wrapper -> master -> ip_addr); 	
			if (RC == 0)
			{
				break;
			}
			sleep(1);
		}
		while ( 1 )
		{
			int port;
			RC = get_chirp_integer(chirp, "MasterPort", &port); 	
			if (RC == 0)
			{
				par_wrapper -> master -> port = (uint16_t) port;
				break;
			}
			sleep(1);
		}
		debug(PRNT_INFO, "Received master address/port: %s:%d\n", par_wrapper -> master -> ip_addr, par_wrapper -> master -> port);	
	}

	/* Done with chirp */
	chirp_client_disconnect(chirp);	
	return 0; /* Success */
}
