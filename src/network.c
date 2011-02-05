/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "log.h"

#include <pthread.h>
#include <semaphore.h>

#define PORT 1500
#define BUFFER_SIZE (1024u)

typedef struct PARALLEL_WRAPPER
{
	unsigned int low_port;
	unsigned int high_port;
	unsigned int command_port; /**< The assigned command port */
	int command_socket; /**< The command socket */
	unsigned int keep_alive_port; /**< The assigned keep alive port */
	int keep_alive_socket; /**< The keep_alive socket */
	sem_t sem; /**< Structure semaphore */
} PARALLEL_WRAPPER;

int get_bound_dgram_socket(int port);
int port_by_range(struct PARALLEL_WRAPPER *par_wrapper, unsigned int *port, int *socketfd);
void * process_command(void *);

int main()
{
	fd_set readfds;
	PARALLEL_WRAPPER *par_wrapper = calloc(1, sizeof(struct PARALLEL_WRAPPER));
	/* Set low range and high range */
	par_wrapper -> low_port = 1024;
	par_wrapper -> high_port = 2048;
	sem_init(&par_wrapper -> sem, 0, 1); /* Semaphore is available */

	/* Open the command port */
	sem_wait(&par_wrapper -> sem);
	port_by_range(par_wrapper, &par_wrapper -> command_port, &par_wrapper -> command_socket);
	port_by_range(par_wrapper, &par_wrapper -> keep_alive_port, &par_wrapper -> keep_alive_socket);
	sem_post(&par_wrapper -> sem);


	FD_ZERO(&readfds);
	FD_SET(par_wrapper -> command_socket, &readfds);
	FD_SET(par_wrapper -> keep_alive_socket, &readfds);
	int n = par_wrapper -> keep_alive_socket + 1;
	while ( 1 )
	{
		select(n, &readfds, NULL, NULL, NULL);
		print(PRNT_INFO, "Returned from select()\n");
		if (FD_ISSET(par_wrapper -> command_socket, &readfds))
		{
			pthread_t thread;
			/* This is the child process */
			pthread_create(&thread, NULL, &process_command, par_wrapper);
		}
		else if (FD_ISSET(par_wrapper -> keep_alive_socket, &readfds))
		{
			/* Respond to keep_alive */
		}
	}

	return 0;
}

void * process_command(void *data)
{
	struct sockaddr_storage from;
	socklen_t from_len = sizeof(struct sockaddr_storage);
	PARALLEL_WRAPPER *par_wrapper = (PARALLEL_WRAPPER *) data;
	if (data == (PARALLEL_WRAPPER *) NULL)
	{
		return NULL;
	}
	print(PRNT_INFO, "Thread started\n");
	/* Allocate a buffer to hold the information */
	char *buffer = (char *) malloc(sizeof(char) * BUFFER_SIZE);
	if (buffer == (char *)NULL)
	{
		print(PRNT_ERR, "Unable to allocate buffer for command processing\n");
		return NULL;
	}
	int num_read = recvfrom(par_wrapper -> command_socket, buffer, BUFFER_SIZE, 0,
		   (struct sockaddr *)&from, &from_len);	

	if (num_read <= 0)
	{
		return NULL;
	}

	printf("Read: %.*s\n", num_read, buffer);

	return NULL;
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
int get_bound_dgram_socket(int port)
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

