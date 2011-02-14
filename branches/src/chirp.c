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
	struct chirp_client *chirp = chirp_client_connect_default();
	if (chirp == (struct chirp_client *)NULL)
	{
		print(PRNT_ERR, "Unable to open chirp context\n");
		return 1;
	}

	RC = get_chirp_integer(chirp, "RequestCpus", &par_wrapper -> this_machine -> cpus);
	if (RC != 0)
	{
		print(PRNT_WARN, "Failed to get RequestCpus, assuming 1\n");
		par_wrapper -> this_machine -> cpus = 1;
	}

	/* Get the initial working directory */
	RC = get_chirp_string(chirp, "Iwd", &par_wrapper -> this_machine -> iwd);
	if (RC != 0)
	{
		par_wrapper -> this_machine -> iwd = getcwd(NULL, 0); /* Allocates space */
		print(PRNT_WARN, "Failed to get Iwd, assuming current directory: %s\n",
				par_wrapper -> this_machine -> iwd);
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
