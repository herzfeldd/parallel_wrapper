#include "wrapper.h"
#include "log.h"

static int send_command_to_sinful(int, char *sinful_string, char *command);
static int send_command_to_host_port(int socketfd, char *hostname, uint16_t port, char *command);
/**
 * Sends a command to the given rank
 */
int send_command_to_rank(PARALLEL_WRAPPER *par_wrapper, int dest_rank, char *command)
{
	if (dest_rank < 0 || dest_rank >= par_wrapper -> num_procs)
	{
		print(PRNT_WARN, "Unable to send command to rank %d, it is not within the allowable range [0, %d)\n", dest_rank, par_wrapper -> num_procs);
		return 2;
	}

	if (par_wrapper -> rank != 0)
	{
		if (dest_rank != 0)
		{
			print(PRNT_WARN, "Unable to send command to rank %d. I am not rank 0 (I don't know their sinful string).\n", dest_rank);
			return 1;
		}
		if (par_wrapper -> rank0_sinful == (char *)NULL)
		{
			print(PRNT_WARN, "Unable to send command to rank 0. Address is unknown.\n");
			return 2;
		}
		/* All good, send the command to rank 0 */
		return send_command_to_sinful(par_wrapper -> command_socket, par_wrapper -> rank0_sinful, command);
	}
	else
	{
		if (par_wrapper -> processors == (struct PROC **)NULL)
		{
			print(PRNT_WARN, "Unable to send command to rank %d the processor table is not initialized.\n", dest_rank);
			return 1;
		}
		if (par_wrapper -> processors[dest_rank] == (struct PROC *)NULL)
		{
			print(PRNT_WARN, "Unable to send command to rank %d, that rank has not yet registered.\n", dest_rank);
			return 2;
		}
		/* All good, send the command */
		return send_command_to_host_port(par_wrapper -> command_socket, par_wrapper -> processors[dest_rank] -> sinful, par_wrapper -> processors[dest_rank] -> command_port, command);
	}
	return 5; /* We should never get here */
}	

/**
 * Wrapper around send_command_to_sinful fro rank 0
 */
static int send_command_to_host_port(int socketfd, char *hostname, uint16_t port, char *command)
{
	char sinful[1024];
	snprintf(sinful, 1024, "%s,%d", hostname, port);
	return send_command_to_sinful(socketfd, sinful, command);
}

static int send_command_to_sinful(int socketfd, char *sinful_string, char *command)
{
	int i;
	int gai_result;
	struct addrinfo hints, *info, *curr;
	if (sinful_string == (char *)NULL || strlen(sinful_string) == 0)
	{
		return 0; /* Do nothing */
	}

	char *hostname = (char *) malloc(strlen(sinful_string));
	int port = 0;	
	//int sockfd = 0;
	char char_port[1024];
	for (i = 0; i < strlen(sinful_string); i++)
	{
		if (sinful_string[i] == ',')
		{
			hostname[i] = '\0';
			break;
		}
		hostname[i] = sinful_string[i];
	}
	if (i >= strlen(sinful_string) - 1)
	{
		print(PRNT_WARN, "Unable to find port in sinful string: %s\n", sinful_string);
		free(hostname);
		return 1;
	}
	errno = 0;
	port = strtol(sinful_string + i + 1, NULL, 10);
	if (errno != 0)
	{
		print(PRNT_WARN, "Unable to parse port in sinful string: %s\n", sinful_string);
		free(hostname);
		return 2;
	}	
	snprintf(char_port, 1023, "%d", port);
		
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_ADDRCONFIG;

	if ((gai_result = getaddrinfo(hostname, char_port, &hints, &info)) != 0)
	{
		print(PRNT_ERR, "getaddrinfo: %s\n", gai_strerror(gai_result));
		free(hostname);
		return 3;
	}
	print(PRNT_INFO, "Sending command '%s' to %s on port %s\n", command, hostname, char_port);
	curr = info;
	while (curr != (struct addrinfo *)NULL)
	{
		/*if ((sockfd = socket(curr -> ai_family, curr -> ai_socktype, curr -> ai_protocol)) < 0)
		{
			curr = curr -> ai_next;
			continue;
		}*/
		/*if (connect(par_wrapper -> command_socket, curr -> ai_addr, curr -> ai_addrlen) < 0)
		{
			//close(sockfd);
			curr = curr -> ai_next;
			continue;
		}
		break; */ 
		int len = sendto(socketfd, command, strlen(command), 0, curr -> ai_addr, curr -> ai_addrlen);
		if (len > 0)
		{
			break;
		}
	}
	if (curr == (struct addrinfo *)NULL)
	{
		print(PRNT_WARN, "Unable to connect to %s\n", sinful_string);
		free(hostname);
		freeaddrinfo(info);
		return 4;
	}

	/* Now that we have connected we can use send */
	/*int bytes_sent = send(sockfd, command, strlen(command), 0);	
	if (bytes_sent != strlen(command))
	{
		print(PRNT_WARN, "Incomplete send of command to %s\n", sinful_string);
	}*/

	//close(sockfd);
	free(hostname);
	freeaddrinfo(info);

	return 0;
}
