/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include "wrapper.h"
#include "log.h"
#include "commands.h"

/* Local Function Prototypes */
static void * process_command(void *data);
static int get_bound_dgram_socket(int port);
static int command_query(PARALLEL_WRAPPER *par_wrapper, char *args, struct sockaddr_storage *from, socklen_t len);
static int command_term(PARALLEL_WRAPPER *par_wrapper, char *args, struct sockaddr_storage *from, socklen_t len);
static int command_register(PARALLEL_WRAPPER *par_wrapper, char *args, struct sockaddr_storage *from, socklen_t len);
static int command_ack(PARALLEL_WRAPPER *par_wrapper, char *args, struct sockaddr_storage *from, socklen_t len);
static char *get_hostname_by_source(struct sockaddr_storage *ss, socklen_t len);
static char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen);

void * service_network(void *data)
{
	pthread_attr_t thread_attr;
	fd_set readfds;
	
	PARALLEL_WRAPPER *par_wrapper = (PARALLEL_WRAPPER *) data;

	/* Set up the thread attributes */
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&thread_attr, BUFFER_SIZE *BUFFER_SIZE);
	
	/* Set up the select statement */
	FD_ZERO(&readfds);
	FD_SET(par_wrapper -> command_socket, &readfds);
	int n = par_wrapper -> command_socket + 1;
	while ( 1 )
	{
		select(n, &readfds, NULL, NULL, NULL);
		if (FD_ISSET(par_wrapper -> command_socket, &readfds))
		{
			MESSAGE *message = calloc(1, sizeof(struct MESSAGE));
			if (message == (MESSAGE *)NULL)
			{
				continue;
			}
			message -> buffer = malloc(sizeof(char) * BUFFER_SIZE);
			if (message -> buffer == (char *)NULL)
			{
				free(message);
				continue;
			}
			message -> par_wrapper = par_wrapper;
			message -> from_len = sizeof(struct sockaddr_storage);
			/* Receive the message */
			message -> buffer_length = recvfrom(par_wrapper -> command_socket, message -> buffer, BUFFER_SIZE, 0,
		   		(struct sockaddr *)&message -> from, &message -> from_len);	
			pthread_t thread;
			/* This is the child thread */
			pthread_create(&thread, &thread_attr, &process_command, message);
		}
	}
	return NULL; /* should never get here */
}



static void * process_command(void *data)
{
	MESSAGE *message = (MESSAGE *)data;
	PARALLEL_WRAPPER *par_wrapper = (PARALLEL_WRAPPER *) message -> par_wrapper;
	if (par_wrapper == (PARALLEL_WRAPPER *) NULL)
	{
		return NULL;
	}
	/* Allocate a buffer to hold the information */
	if (message -> buffer == (char *)NULL)
	{
		print(PRNT_ERR, "Passed message is null.");
		free(message);
		return NULL;
	}
	if (message -> buffer_length <= 0)
	{
		free(message -> buffer);
		free(message);
		return NULL;
	}

	/* Look at the first integer, this is the command */
	char *next = NULL;
	errno = 0;
	enum CMD command = (enum CMD) strtol(message -> buffer, &next, 10);
	if (errno != 0)
	{
		/* An error ocurred in parsing */
		print(PRNT_WARN, "Unable to parse message\n");
		free(message -> buffer);
		free(message);
		return NULL;
	}

	switch (command)
	{
		case CMD_TERM:
			print(PRNT_INFO, "Received command TERM (%d)\n", CMD_TERM);
			command_term(par_wrapper, next, &message -> from, message -> from_len);
			break;
		case CMD_QUERY:
			print(PRNT_INFO, "Received command QUERY (%d)\n", CMD_QUERY);
			command_query(par_wrapper, next, &message -> from, message -> from_len);
			break;
		case CMD_ACK:
			print(PRNT_INFO, "Received command ACK (%d)\n", CMD_ACK);
			command_ack(par_wrapper, next, &message -> from, message -> from_len);
			break;
		case CMD_SEND_FILE:
			print(PRNT_INFO, "Received command SEND_FILE (%d)\n", CMD_SEND_FILE);
			break;
		case CMD_REGISTER:
			print(PRNT_INFO, "Received command REGISTER (%d)\n", CMD_REGISTER);
			command_register(par_wrapper, next, &message -> from, message -> from_len);
			break;
		default:
			print(PRNT_WARN, "Unknown command %d\n", command);
			break;
	}

	free(message -> buffer);
	free(message);
	pthread_exit(NULL);
}

static int command_ack(PARALLEL_WRAPPER *par_wrapper, char *args, struct sockaddr_storage *from, socklen_t len)
{
	/* The assumed structure of the command is ACK <RANK> */
	errno = 0;
	int rank = strtol(args, NULL, 10);
	if (errno != 0)
	{
		print(PRNT_WARN, "Malformed ACK command. Expected ACK <RANK>\n");
		return 1;
	}
	if (rank < 0 || rank >= par_wrapper -> num_procs)
	{
		print(PRNT_WARN, "ACK from rank %d is not valid, must be in range [%d, %d)\n", rank, 0, par_wrapper -> num_procs);
		return 2;	
	}
	
	/* Lock the par_wrapper */
	pthread_mutex_lock(&par_wrapper -> mutex);
	par_wrapper -> ack -> rank = rank;
	par_wrapper -> ack -> received = 1;
	if (par_wrapper -> rank == 0)
	{
		/* Set update time */
		time(&par_wrapper -> processors[rank - 1] -> last_update);
	}
	pthread_mutex_unlock(&par_wrapper -> mutex);
	print(PRNT_INFO, "Successfully recieved ACK from rank %d\n", rank);
	return 0;
}

/**
 * Register a processor
 */
static int command_register(PARALLEL_WRAPPER *par_wrapper, char *args, struct sockaddr_storage *from, socklen_t len)
{
	/* The assumed structure of the command is REGISTER <RANK> <CMD_PORT>*/
	if (par_wrapper -> rank != 0)
	{
		print(PRNT_WARN, "Cannot register. I am rank %d not rank 0\n", par_wrapper -> rank);
		return 1;
	}
	errno = 0;
	int rank = strtol(args, &args, 10);
	if (errno != 0)
	{
		print(PRNT_WARN, "Malformed REGISTER command. Expected REGISTER <RANK> <CMD_PORT>\n");
		return 2;
	}
	if (rank <= 0 || rank >= par_wrapper -> num_procs)
	{
		print(PRNT_WARN, "Cannot register rank %d, must be in range (%d, %d)\n", rank, 0, par_wrapper -> num_procs);
		return 3;
	}
	/* Check if we are already in the list */
	if (par_wrapper -> processors == (struct PROC **)NULL)
	{
		print(PRNT_WARN, "Unable to register rank %d, processor table has not been allocated\n", rank);
		return 4;
	}
	/* Get the command port */
	errno = 0;
	uint16_t cmd_port = strtol(args, NULL, 10);
	if (errno != 0)
	{
		print(PRNT_WARN, "Malformed REGISTER command. Expected REGISTER <RANK> <CMD_PORT>\n");
		return 5;
	}
	/* Sending from random port - we need to response to the command port */
	switch (from -> ss_family)
	{
		case AF_INET:
            ((struct sockaddr_in *)from) -> sin_port = htons(cmd_port);
            break;
        case AF_INET6:
            ((struct sockaddr_in6 *)from) -> sin6_port = htons(cmd_port);
            break;
		default:
			print(PRNT_ERR, "Invalid ss_family\n");
			break;
	}

	/* Attempt to send ACK back */
	char buffer[BUFFER_SIZE];
	sprintf(buffer, "%d %d", CMD_ACK, par_wrapper -> rank);
	int sent = sendto(par_wrapper -> command_socket, buffer, strlen(buffer), 0, (struct sockaddr *)from, len);
	if (sent < 0)
	{
		print(PRNT_ERR, "%s\n", strerror( errno ) );
		return 6;
	}
	
	if (par_wrapper -> processors[rank-1] != (struct PROC *)NULL)
	{
		print(PRNT_WARN, "Rank %d has already been registered. Skipping.\n", rank);
		return 5;
	}
	/* Add to the list (-1's because rank 0 is not in the list)*/
	pthread_mutex_lock(&par_wrapper -> mutex);
	par_wrapper -> processors[rank-1] = (struct PROC *)calloc(1, sizeof(struct PROC));
	par_wrapper -> processors[rank-1] -> sinful = get_hostname_by_source(from, len);
	par_wrapper -> processors[rank-1] -> command_port = cmd_port;
	par_wrapper -> processors[rank-1] -> rank = rank;
	par_wrapper -> registered_processors++;
	/* Set the last update time */
	time(&par_wrapper -> processors[rank-1] -> last_update);
	pthread_mutex_unlock(&par_wrapper -> mutex);
	print(PRNT_INFO, "Successfully registered rank %d\n", rank);
	
	return 0;
}


/**
 * Process the terminate signal
 */
static int command_term(PARALLEL_WRAPPER *par_wrapper, char *args, struct sockaddr_storage *from, socklen_t len)
{
	/* The assumed structure of the command is TERM <RETURN CODE> */
	
	/* Make sure that the command originated from the master (hostname) */
	char *src_hostname = get_hostname_by_source(from, len);
	if (par_wrapper -> rank0_sinful != (char *)NULL)
	{
		/* Get the rank0 hostname */
		if (strcmp(par_wrapper -> rank0_sinful, src_hostname) != 0)
		{
			print(PRNT_WARN, "Unauthorized attempt to shutdown from %s\n", src_hostname);
			return 1;
		}
	}
	free(src_hostname);

	/* Send ACK */
	char buffer[BUFFER_SIZE];
	sprintf(buffer, "%d", CMD_ACK);
	int sent = sendto(par_wrapper -> command_socket, buffer, strlen(buffer), 0, (struct sockaddr *)from, len);
	if (sent < 0)
	{
		print(PRNT_ERR, "%s\n", strerror( errno ) );
		return 2;
	}
	/* Get the return code */
	errno = 0;
	int RC = strtol(args, NULL, 10);
	if (errno == 0)
	{
		cleanup(RC, par_wrapper);
	}
	else
	{
		cleanup(0, par_wrapper); /* Assume no errors */
	}
	return 0;	
}

/**
 * Process the query command
 */
static int command_query(PARALLEL_WRAPPER *par_wrapper, char *args, struct sockaddr_storage *from, socklen_t len)
{
	char buffer[BUFFER_SIZE];

	/* Send the reply (CMD_ALIVE) back to the host */
	sprintf(buffer, "%d", CMD_ACK);
	int sent = sendto(par_wrapper -> command_socket, buffer, strlen(buffer), 0, (struct sockaddr *)from, len);
	if (sent < 0)
	{
		print(PRNT_ERR, "%s\n", strerror( errno ) );
	}
	return 0;
}	


/**
 * Obtain a port in the range specified by par_wrapper
 */
int port_by_range(struct PARALLEL_WRAPPER *par_wrapper, uint16_t *port, int *socketfd)
{
	int i;
	/* Check the validity of par_wrapper */
	if (par_wrapper == (PARALLEL_WRAPPER *)NULL)
	{
		return 1;
	}
	if (par_wrapper -> low_port >= par_wrapper -> high_port)
	{
		return 2;
	}
	/* Check the validity of port and socket */
	if ((port == (uint16_t *) NULL) || (socketfd == (int *)NULL))
	{
		return 3;
	}
	for (i = par_wrapper -> low_port; i < par_wrapper -> high_port; i++)
	{
		*socketfd = get_bound_dgram_socket(i);
		if (*socketfd > 0)
		{
			*port = i; /* The bound port number */
			break; /* We bound to a port */
		}
	}
	if (*socketfd <= 0)
	{
		print(PRNT_ERR, "Unable to bind to a port in range %d:%d\n", 
				par_wrapper -> low_port, par_wrapper -> high_port);
		return 3; /* We failed to bind to a socket */
	}
	debug(PRNT_INFO, "Bound port to %d\n", *port);

	return 0;
}


/**
 * Returns the socket descriptor associated with the passed port. 
 * If an error occurs, the function returns <= 0.
 */
static int get_bound_dgram_socket(int port)
{
	int socketfd, RC;
	struct addrinfo hints;
	struct addrinfo *servinfo, *curr; /* Points to the results */
	char char_port[255];
	/* Print the port into the char array */
	snprintf(char_port, 254, "%d", port);
	if (port <= 0)
	{
		return -1;
	}
	
	/* Clear out the addrinfo data structure */
	memset(&hints, 0, sizeof(struct addrinfo));
	
	/* Bind to local host */
	hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* UDP Packets */
	hints.ai_flags = AI_PASSIVE; /* Fill in my IP For me */
	if ((RC = getaddrinfo(NULL, char_port, &hints, &servinfo)) != 0)
	{
		return -2;
	}

	/* Loop through all results, binding to the first interface */
	curr = servinfo;
	while (curr != (struct addrinfo *)NULL)
	{
		if ((socketfd = socket(curr -> ai_family, curr -> ai_socktype, curr -> ai_protocol)) < 0)
		{
			/* We failed - advance to the next one */
			curr = curr -> ai_next;
			continue;
		}
		if (bind(socketfd, curr -> ai_addr, curr -> ai_addrlen) < 0)
		{
			close(socketfd);
		}
		else
		{
			/* Successfully bound, return the socket */
			freeaddrinfo(servinfo);
			return socketfd;
		}
		curr = curr -> ai_next;
	} /* end loop over results */
	
	freeaddrinfo(servinfo);
	return -3; /* We didn't bind to a socket */
}

/**
 * returns the sinful string with the appropriate port
 */
char *get_hostname_sinful_string(int port)
{
	/* Get the hostname */
	char host[1024];
	gethostname(host, 1024);

	print(PRNT_INFO, "%s\n", host);
	char *sinful = malloc(sizeof(char) * BUFFER_SIZE);
	int RC;

	struct addrinfo hints, *info, *curr;
	memset(&hints, 0, sizeof(hints));
	/* Bind to local host */
	hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* UDP Packets */
    hints.ai_flags = AI_ADDRCONFIG; 
    if ((RC = getaddrinfo(host, NULL, &hints, &info)) != 0)
    {
        return NULL;
    }
	for (curr = info; curr != NULL; curr = curr -> ai_next)
	{
	 	get_ip_str((const struct sockaddr *)curr -> ai_addr, sinful, INET6_ADDRSTRLEN);
	}
	snprintf(host, 1024, ",%d", port); 
	strcat(sinful, host);
	return sinful;
}

static char *get_hostname_by_source(struct sockaddr_storage *ss, socklen_t len)
{
	int RC = 0;
	char *name = (char *) malloc(BUFFER_SIZE *sizeof(char));
	if (name == (char *)NULL)
	{
		return NULL;
	}
	
	/* getnameinfo() case. NI_NUMERICHOST avoids DNS lookup. */
	RC = getnameinfo((struct sockaddr *)ss, len,
		name, BUFFER_SIZE, NULL, 0, NI_NUMERICHOST);
	return name;
}

static char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen)
{
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                    s, maxlen);
            break;

        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                    s, maxlen);
            break;

        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
    }

    return s;
}

