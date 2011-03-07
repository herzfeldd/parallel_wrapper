#include "wrapper.h"
#include "string_util.h"
#include <getopt.h>

/**
 * Parse environment variables
 */
void parse_environment_vars(parallel_wrapper *par_wrapper)
{
	char *value = NULL;
	value = getenv("_CONDOR_PROCNO");
	parse_integer(value, &par_wrapper -> this_machine -> rank);

	value = getenv("_CONDOR_NPROCS");
	parse_integer(value, &par_wrapper -> num_procs);

	return;
}

/**
 * Set environment variable values (particularly the path)
 */
void set_environment_vars(parallel_wrapper *par_wrapper)
{
	/* Get the path variable */
	char *temp = getenv("PATH");
	char *path = (char *) calloc(2048, sizeof(char));
	if (path == (char *)NULL)
	{
		print(PRNT_WARN, "Unable to allocate space for new path variable\n");
		return;
	}
	snprintf(path, 2048, "%s:/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin:.", temp == (char *)NULL ? "" : temp);
	setenv("PATH", path, 1); /* Replace path */
	free(path);
}

/**
 * Parse commandline arguments
 */
int parse_args(int argc, char **argv, parallel_wrapper *par_wrapper)
{
	char c;
	while ( 1 )
	{
		static struct option long_options[] = 
		{
			{"verbose", no_argument, &verbose, 1},
			{"help", no_argument, 0, 'h'},
			{"rank", required_argument, 0, 'r'},
			{"ports", required_argument, 0, 'p'},
			{"timeout", required_argument, 0, 't'},
			{"ka-interval", required_argument, 0, 'k'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
		/* The '+' make sure all arguments are processed in order */
		c =getopt_long(argc, argv, "+hr:p:n:t:k:",
			   long_options, &option_index);
		/* Detect the end of the options */
		if (c == -1)
		{
			break;
		}
		int RC;
		switch (c)
		{
			case 0:
				/* If this option set a flag, do nothing else now */
				if (long_options[option_index].flag != 0)
					break;
				printf("option %s", long_options[option_index].name);
				if (optarg)
					printf(" with arg %s\n", optarg);
				printf("\n");
				break;
			case 'h': /* Help */
				help();
				exit(0); /* Leave */
				break;
			case 'n': /* Number of processors */
				RC = parse_integer(optarg, &par_wrapper -> num_procs);
				if (RC != 0)
				{
					print(PRNT_ERR, "Unable to parse the number of processors\n");
					help();
					exit(1);
				}
				break;
			case 'r': /* Rank */
				/* Make sure we can correctly parse the arg */
				RC = parse_integer(optarg, &par_wrapper -> this_machine -> rank);
				if (RC != 0)
				{
					print(PRNT_ERR, "Unable to parse the rank.\n");
					help();
					exit(1);
				}
				break;
			case 'p': /* Port Range {low:high} */
				/* Attempt to parse */
				print(PRNT_WARN, "Port range not implemented\n");
				break;
			case 't': /* Timeout */
				/* Attempt to parse */
				RC = parse_integer(optarg, &par_wrapper -> timeout);
				if (RC != 0)
				{
					print(PRNT_ERR, "Unable to parse timeout\n");
					help();
					exit(1);
				}
				if (par_wrapper -> timeout < 10)
				{
					print(PRNT_WARN, "Assuming minimum timeout of 10 seconds.\n");
					par_wrapper -> timeout = 10;
				}
				break;
			case 'k': /* Keep-alive interval */
				/* Attempt to parse */
				RC = parse_integer(optarg, &par_wrapper -> ka_interval);
				if (RC != 0)
				{
					print(PRNT_ERR, "Unable to parse keep-alive interval\n");
					help();
					exit(1);
				}
				if (par_wrapper -> ka_interval < 1)
				{
					print(PRNT_WARN, "Assuming minimum keep-alive interval of 1 seconds\n");
					par_wrapper -> ka_interval = 1;
				}
				break;	
			default:
				printf("\n");
				help();
				exit(4);
				break;
		}
	}	
	if (optind >= argc)
	{
		print(PRNT_ERR, "No executable passed to the wrapper\n");
		exit(2);
	}

	/** 
	 * The length of the executable and its arguments is given by
	 * argc - optind
	 */
	par_wrapper -> executable_length = argc - optind;
	/* Add one so that we have a null terminator */
	par_wrapper -> executable = (char **) calloc(par_wrapper -> executable_length + 1, sizeof(char *));
	int i;
	for (i = 0; i < par_wrapper -> executable_length; i++)
	{
		par_wrapper -> executable[i] = strdup(argv[optind + i]);
	}
	return 0;
}

/**
 * Help functionality
 */
void help(void)
{
	printf("Usage:\n");
	printf(" parallel_wrapper [options] executable [args]\n");
	printf("\n");
	printf("Flags:\n");
	printf(" --verbose                  verbose mode\n");
	printf("\n");
	
	printf("Options:\n");
	printf(" -h, --help                 this help message\n");
	printf(" -r, --rank={value}         set the rank of this host\n");
	printf(" -n {value}                 number of processes\n");
	printf(" -p, --ports={low:high}     port range to use\n");
	printf("\n");

	printf("Environment Variables:\n");
	printf(" [_CONDOR_PROCNO]           set the rank of this host\n");
	printf(" [_CONDOR_NUMPROCS]         number of processes\n");	
	printf("\n");
	
	printf("Environment Variables Set By Wrapper:\n");
	printf(" [MACHINE_FILE]             the location of the machine file\n");
	printf(" [HOST_FILE]                identical to [MACHINE_FILE]\n");
	printf(" [SSH_CONFIG]               location of the SSH_CONFIG file\n");
	printf(" [SSH_WRAPPER]              wrapper around ssh which uses [SSH_CONFIG]\n");
	printf(" [REQUEST_CPUS]             the number of LOCAL cpus allocated (usually 1)\n");
	printf(" [NUM_MACHINES]             the number of machines (num_machines)\n");
	printf(" [NUM_PROCS]                total CPUs allocated for this task. This is\n");
	printf("                            the sum of request cpus across all machines.\n");
	printf(" [CPUS]                     identical to [NUM_PROC]\n");
	printf(" [RANK]                     the rank of this process (MASTER)\n");
	printf(" [CLUSTER_ID]               the Condor cluster ID\n");
	printf(" [SCRATCH_DIR]              wrapper scratch directory\n");
	printf(" [IWD]                      the job's initial working directory on the startd\n");
	printf(" [SCHEDD_IWD]               the job's initial working directory on the schedd\n");
	printf(" [IP_ADDR]                  the IP address of this host\n");
	printf(" [CMD_PORT]                 the command port on this host\n");
	printf(" [TRANSFER_FILES]           'TRUE'/'FALSE' depending on if the \n");
	printf("                            IWD is shared across all hosts \n");
	printf(" [SHARED_FS]                the location of the shared FS for\n");
	printf("                            this MPI process. For jobs which have\n");
	printf("                            [TRANSFER_FILES]='FALSE', this is a 'fake'\n");
	printf("                            shared FS\n");
	printf(" [SHARED_DIR]               identical to [SHARED_FS]\n");
	printf("\n");
	printf("\n");
	
	return;
}
