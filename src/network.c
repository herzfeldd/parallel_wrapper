/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include "wrapper.h"
#include "log.h"
#include "commands.h"

/* Local Function Prototypes */
static void * process_command(void *data);
static int get_bound_dgram_socket(int port);
static int command_query(PARALLEL_WRAPPER *par_wrapper, struct sockaddr_storage *from, socklen_t len);

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
		print(PRNT_INFO, "Returned from select()\n");
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
	print(PRNT_INFO, "Thread started\n");
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
			break;
		case CMD_QUERY:
			print(PRNT_INFO, "Received command QUERY (%d)\n", CMD_QUERY);
			command_query(par_wrapper, &message -> from, message -> from_len);
			break;
		case CMD_ALIVE:
			print(PRNT_INFO, "Received command ALIVE (%d)\n", CMD_ALIVE);
			break;
		case CMD_SEND_FILE:
			print(PRNT_INFO, "Received command SEND_FILE (%d)\n", CMD_SEND_FILE);
			break;
		default:
			print(PRNT_WARN, "Unknown command %d\n", command);
			break;
	}

	free(message -> buffer);
	free(message);
	pthread_exit(NULL);
}

static int command_query(PARALLEL_WRAPPER *par_wrapper, struct sockaddr_storage *from, socklen_t len)
{
	char buffer[BUFFER_SIZE];

	/* Send the reply (CMD_ALIVE) back to the host */
	sprintf(buffer, "%d", CMD_ALIVE);
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
int port_by_range(struct PARALLEL_WRAPPER *par_wrapper, unsigned int *port, int *socketfd)
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
	if ((port == (unsigned int *) NULL) || (socketfd == (int *)NULL))
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
char *get_sinful_string(int port)
{
	struct addrinfo hints, *info;
	int gai_result;
	char char_port[1024];
	snprintf(char_port, 1024, "%d", port);
	char *fqdn = (char *) malloc(BUFFER_SIZE *sizeof(char));
	if (fqdn == (char *)NULL)
	{
		return NULL;
	}

	fqdn[BUFFER_SIZE-1] = '\0';
	gethostname(fqdn, BUFFER_SIZE-1);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; /*either IPV4 or IPV6*/
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;

	if ((gai_result = getaddrinfo(fqdn, char_port, &hints, &info)) != 0) 
	{
    	print(PRNT_ERR, "getaddrinfo: %s\n", gai_strerror(gai_result));
		snprintf(fqdn, BUFFER_SIZE, "%s:%d", fqdn, port);
		return fqdn;
	}
	if (info -> ai_canonname != NULL)
	{
		snprintf(fqdn, BUFFER_SIZE, "%s:%d", info -> ai_canonname, port);
	}
	return fqdn;	
}
