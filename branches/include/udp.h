#ifndef UDP_H
#define UDP_H

#include "network_util.h"
#include <setjmp.h>

typedef enum CMD
{
	CMD_NULL = 0, /**< NULL command */
	CMD_TERM = 10, /**< Terminate */
	CMD_QUERY, /**< Query status */
	CMD_ACK, /**< I am alive */
	CMD_SEND_FILE, /**< Send a file to this TCP port */
	CMD_REGISTER, /**< Register to rank 0 */
	CMD_CREATE_LINK /**< Create a soft link */
} CMD;

extern int jmpset;
extern sigjmp_buf jmpbuf;

extern void *udp_server(void *ptr);
extern int ack(int socketfd, int rank, const struct sockaddr *addr);
extern int term(int socketfd, int return_code, char *ip_addr, uint16_t port);

#endif /* UDP_H */
