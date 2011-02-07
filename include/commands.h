#ifndef NETWORK_H
#define NETWORK_H

typedef enum CMD
{
	CMD_TERM = 10, /**< Terminate */
	CMD_QUERY, /**< Query status */
	CMD_ACK, /**< I am alive */
	CMD_SEND_FILE, /**< Send a file to this TCP port */
	CMD_REGISTER /**< Register to rank 0 */
} CMD;

extern void * service_network(void *);
extern int port_by_range(struct PARALLEL_WRAPPER *par_wrapper, unsigned int *port, int *socketfd);
extern char *get_hostname_sinful_string(int port);

#endif /* COMMAND_H */
