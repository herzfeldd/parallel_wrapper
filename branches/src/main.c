
#include <stdio.h>
#include "chirp_util.h"
#include "string_util.h"
#include "timer.h"
#include "log.h"

int main(int argc, char **argv)
{
	struct chirp_client *chirp = chirp_client_connect_default();
	if (chirp == (struct chirp_client *)NULL)
	{
		print(PRNT_ERR, "Unable to open chirp context\n");
		return 1;
	}

	int RC;
	int cpus = 0;
	RC = get_chirp_integer(chirp, "RequestCpus", &cpus);
   	if (RC == 0)
	{
		print(PRNT_INFO, "Got RequestCpus = %d\n", cpus);
	}	
	else
	{
		print(PRNT_ERR, "Failed to get RequestCpus, RC = %d\n", RC);
	}

	/* Get the initial working directory */
	char *iwd = NULL;
	RC = get_chirp_string(chirp, "Iwd", &iwd);
	if (RC == 0)
	{
		print(PRNT_INFO, "Got Iwd = %s\n", iwd);
	}	
	else
	{
		print(PRNT_ERR, "Failed to get Iwd, RC = %d\n", RC);
	}

	char *junk = NULL;
	RC = get_chirp_string(chirp, "Junk", &junk);
	if (RC == 0)
	{
		print(PRNT_INFO, "Got Junk = %s\n", &junk);
	}	
	else
	{
		print(PRNT_ERR, "Failed to get Junk, RC = %d\n", RC);
	}
	return 0;
}
