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

#define PORT 1500

int get_bound_dgram_socket(int port);
int main()
{
	int i, socketfd;
	for (i = 1; i < 2048; i++)
	{
		socketfd = get_bound_dgram_socket(i);
		if (socketfd > 0)
		{
			break;
		}
	}
	printf("Successfully bound to port %d\n", i);
	close(socketfd);
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

