#ifndef UDP_H
#define UDP_H

typedef enum CMD
{
	CMD_TERM = 10, /**< Terminate */
	CMD_QUERY, /**< Query status */
	CMD_ACK, /**< I am alive */
	CMD_SEND_FILE, /**< Send a file to this TCP port */
	CMD_REGISTER, /**< Register to rank 0 */
	CMD_CREATE_LINK /**< Create a soft link */
} CMD;

extern void *udp_server(void *ptr);

#endif /* UDP_H */
