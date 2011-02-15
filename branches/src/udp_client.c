#include "wrapper.h"
#include "network_util.h"
#include "string_util.h"

/**
 * Send an ACK back to the host specified in the sockaddr structure
 *
 * @param socketfd The socket to send the message on
 * @param rank The rank of the current host 
 * @param sockaddr The socket to reply to 
 * @return 0 on success, otherwise failure
 */
int ack(int socketfd, int rank, const struct sockaddr *addr)
{
	int RC = 0;
	if (addr == (struct sockaddr *)NULL)
	{
		print(PRNT_WARN, "Socket address is null\n");
		return 1;
	}
	if (rank < 0)
	{
		print(PRNT_WARN, "Invalid rank\n");
		return 2;
	}
	if (socketfd < 0)
	{
		print(PRNT_WARN, "Invalid socket descriptor\n");
		return 3;
	}
	char message[1024];
	snprintf(message, 1024, "%d:%d\n", CMD_ACK, rank);
	RC = send_string_reply(addr, message, socketfd);
	return RC;
}

/**
 * Send the term signal to a host with the given ip_addr and port.
 *
 * Sends the TERM command to a host with the given ip and port. A
 * return code is also sent with the packet.
 *
 * @param socketfd The socket to send the message on 
 * @param return_code The return code to send with the TERM signal
 * @param ip_addr The ipaddress of the receiving server
 * @param port The port of the receiving server 
 */
int term(int socketfd, int return_code, char *ip_addr, uint16_t port)
{
	int RC = 0;
	if (ip_addr == (char *)NULL)
	{
		print(PRNT_WARN, "Invalid IP address\n");
		return 1;
	}

	char message[1024];
	snprintf(message, 1024, "%d:%d\n", CMD_TERM, return_code);
	RC = send_string_to_ip_port(ip_addr, port, message, socketfd);
	return RC;
}
