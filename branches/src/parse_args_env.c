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

	value = getenv("MPI_EXEC");
	if (value != (char *) NULL)
	{
		if (par_wrapper -> mpi_executable != (char *)NULL)
		{
			free(par_wrapper -> mpi_executable);
		}
		par_wrapper -> mpi_executable = strdup(value); 
	}

	value = getenv("MPI_FLAGS");
	if (value != (char *)NULL)
	{
		if (par_wrapper -> mpi_flags != (char *)NULL)
		{
			free(par_wrapper -> mpi_flags);
		}
		par_wrapper -> mpi_flags = strdup(value); 
	}
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
			{"flags", required_argument, 0, 'f'},
			{"mpi", required_argument, 0, 'm'},
			{"ports", required_argument, 0, 'p'},
			{"scratch", required_argument, 0, 's'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
		/* The '+' make sure all arguments are processed in order */
		c =getopt_long(argc, argv, "+hr:f:m:p:s:n:",
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
					print(PRNT_ERR, "Unable to parse the number of processors\n");
					help();
					exit(1);
				}
				break;
			case 'f': /* Flags */
				if (par_wrapper -> mpi_flags != (char *)NULL)
				{
					free(par_wrapper -> mpi_flags);
				}
				par_wrapper -> mpi_flags = strdup(optarg);
				break;
			case 'm': /* MPI Executable */
				if (par_wrapper -> mpi_executable != (char *)NULL)
				{
					free(par_wrapper -> mpi_executable);
				}
				par_wrapper -> mpi_executable = strdup(optarg); 
				break;
			case 'p': /* Port Range {low:high} */
				/* Attempt to parse */
				print(PRNT_WARN, "Port range not implemented\n");
				break;
			case 's': /* Scratch Directory */
				print(PRNT_WARN, "Scratch directory not implemented\n");
				/*if (par_wrapper -> scratch_dir != (char *)NULL)
				{
					free(par_wrapper -> scratch_dir);
				}	
				par_wrapper -> scratch_dir = strdup(optarg); */
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
	printf("Options:\n");
	printf(" -h, --help                 this help message\n");
	printf(" -r, --rank={value}         set the rank of this host\n");
	printf(" -n {value}                 number of processes\n");
	printf(" --verbose                  verbose mode\n");
	printf(" -f, --flags={value}        flags to pass to mpi\n");
	printf(" -m, --mpi={path}           path to the mpi executable\n");
	printf(" -p, --ports={low:high}     port range to use\n");
	printf(" -s, --scratch={path}       scratch directory to use\n");

	printf("\n");
	printf("Environment Variables:\n");
	printf(" [_CONDOR_PROCNO]           set the rank of this host\n");
	printf(" [_CONDOR_NUMPROCS]         number of processes\n");	
	printf(" [MPI_FLAGS]                flags to pass to mpi\n");
	printf(" [MPI_EXEC]                 path to the mpi executable\n");
	return;
}
