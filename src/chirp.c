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

	RC = get_chirp_integer(chirp, "EnteredCurrentStatus", &par_wrapper -> entered_current_status);
	if (RC != 0)
	{
		print(PRNT_WARN, "Failed to get EnteredCurrentStatus from classad.\n");
		return 2;
	}
	
	par_wrapper -> this_machine -> schedd_iwd = get_chirp_string(chirp, "IWD");
	if (par_wrapper -> this_machine -> schedd_iwd == NULL)
	{
		print(PRNT_WARN, "Failed to get IWD from classad. Assuming the current directory.");
		par_wrapper -> this_machine -> schedd_iwd = (char *) malloc(1024 * sizeof(char));
		getcwd(par_wrapper -> this_machine -> schedd_iwd, 1024);
	}	

	/* Send the MASTER information back to the schedd */
	if (par_wrapper -> this_machine -> rank == MASTER)
	{
		/* Get a random number */
		srand (time(NULL)); /* Initialize random generator */
		int random = rand() % SHRT_MAX;
		char temp_str[1024];
		char temp_str2[1024];
		snprintf(temp_str2, 1024, "%d", random);
		RC = chirp_client_set_job_attr(chirp, "RandInt", temp_str2);
		if (RC != 0)
		{
			print(PRNT_ERR, "Unable to send RandInt to the chirp server\n");
			return 3;
		}

		snprintf(temp_str, 1024, "\"%s\"", par_wrapper -> this_machine -> ip_addr);
		snprintf(temp_str2, 1024, "MasterIpAddr_%d", random);
		RC = chirp_client_set_job_attr(chirp, temp_str2,  temp_str);
		if (RC != 0)
		{
			print(PRNT_ERR, "Unable to send MasterIpAddr to the chirp server\n");
			return 3;
		}
		snprintf(temp_str2, 1024, "MasterPort_%d", random);
		snprintf(temp_str, 1024, "%d", par_wrapper -> this_machine -> port);
		RC = chirp_client_set_job_attr(chirp, temp_str2, temp_str);
		if (RC != 0)
		{
			print(PRNT_ERR, "Unable to send MasterPort to the chirp server\n");
			return 4;
		}
	}
	else /* I am not the master */
	{
		char temp_str2[1024];
		int random;
		int port;
		print(PRNT_INFO, "Attempting to get Host/IP from the schedd\n");
		while ( 1 )
		{
			sleep(1);
			RC = get_chirp_integer(chirp, "RandInt", &random);
			if (RC != 0)
			{
				continue;
			}
			snprintf(temp_str2, 1024, "MasterIpAddr_%d", random);
			par_wrapper -> master -> ip_addr = get_chirp_string(chirp, temp_str2); 	
			if (par_wrapper -> master -> ip_addr == (char *)NULL)
			{
				continue;
			}
			snprintf(temp_str2, 1024, "MasterPort_%d", random);
			RC = get_chirp_integer(chirp, temp_str2, &port); 	
			if (RC != 0)
			{
				continue;
			}
			/* Store the port */
			par_wrapper -> master -> port = (uint16_t) port;
			break; /* We have everything we need */
		}

		debug(PRNT_INFO, "Received master address/port: %s:%d\n", par_wrapper -> master -> ip_addr, par_wrapper -> master -> port);	
	}

	/* Done with chirp */
	chirp_client_disconnect(chirp);	
	return 0; /* Success */
}
