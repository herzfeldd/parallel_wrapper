/**
 * Network utility functions
 */

#include "network_util.h"
#include "log.h"

/**
 * Returns the IP address associated with this host.
 *
 * The IP address returned is (hopefully) not local, although this is
 * not gauranteed
 *
 * @return An allocated string containing the IP address of this host
 *   returns NULL if there is an allocation error.
 */
char *get_ip_addr(void)
{
	char hostname[1024]; /* Hostname filled in here */
	struct addrinfo hints, *info, *curr;
	gethostname(hostname, 1024);
	
	/* Allocate a buffer to hold (at most) and IPv6 string */
	char *ip_addr = malloc(sizeof(char) * INET6_ADDRSTRLEN);
   	if (ip_addr == (char *)NULL)
	{
		print(PRNT_ERR, "Unable to allocate ip_addr string\n");
		return NULL;
	}	
	
	memset(&hints, 0, sizeof(hints));
	//hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
	hints.ai_family = AF_INET; /* IPv4 ONLY */
	hints.ai_socktype = SOCK_DGRAM; /* UDP packets */
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; 
	if (getaddrinfo(hostname, NULL, &hints, &info) != 0)
	{
		print(PRNT_ERR, "Unable to get addrinfo for hostname %s\n", hostname);
		free(ip_addr);
		return NULL;
	}
	/* Get to the last value */
	for (curr = info; curr -> ai_next != NULL; curr = curr -> ai_next)
	{
		; /* Do nothing */
	}
	/* Get the IP string */
	if (ip_str_from_sockaddr((struct sockaddr *)curr -> ai_addr, ip_addr, INET6_ADDRSTRLEN))
	{
		print(PRNT_ERR, "Unable to get IP addr string\n");
		free(ip_addr);
		freeaddrinfo(info);
		return NULL;
	}
	freeaddrinfo(info);
	return ip_addr;
}

/**
 * Returns the IP representation of the address in addr
 *
 * Returns the IP (v4 or v6) representation of the sockaddr in addr into
 * the preallocated buffer of length buffer_len
 *
 * @param addr A sockaddr structure containin the address to conver to a string
 * @param buffer The buffer to place the ulimate IP address in
 * @param buffer_len The length of the buffer
 * @return 0 if SUCCESS otherwise failure
 */
int ip_str_from_sockaddr(const struct sockaddr *addr, char *buffer, size_t buffer_len)
{
	if (addr == (struct sockaddr *)NULL)
	{
		print(PRNT_ERR, "NULL sockaddr passed\n");
		return 1;
	}
	if (buffer == (char *)NULL)
	{
		print(PRNT_ERR, "NULL buffer passed\n");
		return 2;
	}
	/* Zero out the contents of the buffer */
	memset(buffer, 0, buffer_len);
	switch (addr -> sa_family)
	{
		case AF_INET:
			if (buffer_len < INET_ADDRSTRLEN)
			{
				print(PRNT_ERR, "Insufficient buffer space for IP conversion\n");
				return 3;
			}
            inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr),
                    buffer, INET_ADDRSTRLEN);
			break;
		case AF_INET6:
			if (buffer_len < INET6_ADDRSTRLEN)
			{
				print(PRNT_ERR, "Insufficient buffer space for IP conversion\n");
				return 4;
			}	
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)addr)->sin6_addr),
                    buffer, INET6_ADDRSTRLEN);
			break;
		default:
			print(PRNT_ERR, "Unknown address format\n");
			return 3;
	}
	return 0; /* Success */
}

/**
 * Returns the port associated with the sockaddr structure
 *
 * Returns the port associated with the sockaddr structure 
 *
 * @param addr A sockaddr structure containin the address to conver to a string
 * @return The port associated with the addr structure (0 on failure)
 */
uint16_t port_from_sockaddr(const struct sockaddr *addr)
{
	uint16_t port = 0;
	if (addr == (struct sockaddr *)NULL)
	{
		print(PRNT_ERR, "NULL sockaddr passed\n");
		return 0; /* Not a valid port */
	}
	switch (addr -> sa_family)
	{
		case AF_INET:
            port = ntohs(((struct sockaddr_in *)addr)->sin_port);
			break;
		case AF_INET6:
            port = ntohs(((struct sockaddr_in6 *)addr)->sin6_port);
			break;
		default:
			print(PRNT_ERR, "Unknown address format\n");
			return 0; /* Not a valid port */
	}
	return port;
}

/**
 * Returns the socket associated with the passed port.
 *
 * If an error occurs (such as being unable to bind to the port, the
 * funtion returns < 0.
 *
 * @return A positive file descriptor on success
 */
int get_bound_dgram_socket(uint16_t port)
{
	int socketfd = -1;	
	struct addrinfo hints, *info, *curr;
	char char_port[255];
	snprintf(char_port, 255, "%u", port);	

	/* Clear out the addrinfo data structure */
	memset(&hints, 0, sizeof(struct addrinfo));
	
	/* Bind to local host */
	//hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
	hints.ai_family = AF_INET; /* IPv4 Only */
	hints.ai_socktype = SOCK_DGRAM; /* UDP Packets */
	hints.ai_flags = AI_PASSIVE; /* Fill in my IP For me */
	if (getaddrinfo(NULL, char_port, &hints, &info) != 0)
	{
		return -1;
	}
	/* Loop through the results, bind to the first available interface */
	for (curr = info; curr != NULL; curr = curr -> ai_next)
	{
		if ((socketfd = socket(curr -> ai_family, curr -> ai_socktype,
						curr -> ai_protocol)) < 0)
		{
			continue;
		}
		if (bind(socketfd, curr -> ai_addr, curr -> ai_addrlen) < 0)
		{
			close(socketfd);
			continue;
		}
		/* If we made it here, we successfully bound */
		freeaddrinfo(info);
		return socketfd;
	}
	/* We failed to bind */
	freeaddrinfo(info);
	return -2;
}

/**
 * Returns a bound DGRAM socket in the range [start, end]
 *
 * Returns a bound datagram socket in the range [start, end]. If no successful binds
 * are possible, the function returns a value != 0.
 *
 * @param start The starting port in the range
 * @param end The ending port in the range
 * @param port (output) The final bound port
 * @param socket (output) The final socket file descriptor
 * @return 0 on SUCCESS, otherwise failure
 */
int get_bound_dgram_socket_by_range(uint16_t start, uint16_t end, uint16_t *port, int *socketfd)
{
	uint16_t i;
	if (port == (uint16_t *)NULL || socketfd == (int *)NULL)
	{
		print(PRNT_ERR, "Invalid port of socketfd\n");
		return 1;
	}
	if (start > end)
	{
		uint16_t temp = start;
		start = end;
		end = temp; 
	}
	for (i = start; i < end; i++)
	{
		*socketfd = get_bound_dgram_socket(i);
		if (*socketfd > 0) /* We successfully bound */
		{
			*port = i;
			return 0;
		}
	}
	print(PRNT_ERR, "Unable to bind to DGRAM port in range [%u, %u]\n", start, end);
	return 1; /* Unable to bind to port in range */
}

/**
 * Sends a string to the destination IP and PORT via UDP
 *
 * Sends a null terminated string to an IP address on a particular port. The
 * passed socket file descriptor is used to send the string.
 *
 * @param ip A string containing the IPv4 or IPv6 (or hostname) of the dest
 * @param port The destination port
 * @param string The string to send
 * @param socketfd The bound socket to send the string on
 * @return 0 if success, otherwise failure
 */
int send_string_to_ip_port(char *ip, uint16_t port, char *string, int socketfd)
{	
	int str_len = 0;
	int gai_result;
	struct addrinfo hints, *info, *curr;
	char char_port[256];
	snprintf(char_port, 256, "%u", port);
	if (ip == (char *)NULL)
	{
		print(PRNT_ERR, "Null destination IP address\n");
		return 1;
	}	
	if (string == (char *)NULL)
	{
		/* Nothing to send */
		return 0;
	}
	str_len = strlen(string);
	if (socketfd <= 0)
	{
		print(PRNT_ERR, "Invalid socket file descriptor\n");
		return 2;
	}
	/* Attempt to send the string */
	memset(&hints, 0, sizeof(hints));
	
	//hints.ai_family = AF_UNSPEC;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_ADDRCONFIG;
	if ((gai_result = getaddrinfo(ip, char_port, &hints, &info)) != 0)
	{
		print(PRNT_ERR, "Unable to send command '%s' to %s:%s. %s\n", 
			   string, ip, char_port, gai_strerror(gai_result));	
		return 3;
	}
	for (curr = info; curr != NULL; curr = curr -> ai_next)
	{
		int length = sendto(socketfd, string, str_len, 0, curr -> ai_addr,
					curr -> ai_addrlen);
		if (length == str_len)
		{
			freeaddrinfo(info);
			return 0;
		}
	}
	freeaddrinfo(info);
	print(PRNT_WARN, "Unable to send command '%s' to %s:%s. Length error.\n", 
			string, ip, char_port);
	return 1;
}

/**
 * Sends the string to the IP address and port contained in addr
 *
 * Sends a reply to the IP and port contained in the addr of the
 * passed sockaddr structure.
 *
 * @param addr A sock addr structure
 * @param string The string to send
 * @param socketfd The socket to send the message on
 * @return 0 on success, otherwise failure
 */
int send_string_reply(const struct sockaddr *addr, char *string, int socketfd)
{
	if (addr == (struct sockaddr *)NULL)
	{
		print(PRNT_ERR, "sockaddr structure is not valid\n");
		return 1;
	}	
	if (string == (char *)NULL)
	{
		return 0; /* Nothing to do */
	}

	/* Get the IP address and the port to send the reply */
	char *ip_addr = (char *)malloc(sizeof(char) * INET6_ADDRSTRLEN);
	if (ip_addr == (char *)NULL)
	{
		print(PRNT_ERR, "Unable to allocate buffer for IP address\n");
	}
	if (ip_str_from_sockaddr(addr, ip_addr, INET6_ADDRSTRLEN) != 0)
	{
		print(PRNT_ERR, "Unable to retrieve IP address from addr\n");
		free(ip_addr);
		return 2;
	}
	/* Get the port */
	uint16_t port = port_from_sockaddr(addr);
	if (port == 0)
	{
		print(PRNT_ERR, "Invalid port in sockaddr structure\n");
		free(ip_addr);
		return 3;
	}
	/* Send the string */

	int RC = send_string_to_ip_port(ip_addr, port, string, socketfd);
	free(ip_addr);
	return RC;
}
