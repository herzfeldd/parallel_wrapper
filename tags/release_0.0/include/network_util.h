#ifndef NETWORK_UTIL_H
#define NETWORK_UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

extern char *get_ip_addr(void);
extern int ip_str_from_sockaddr(const struct sockaddr *addr, char *buffer, size_t buffer_len);
extern int get_bound_dgram_socket(uint16_t port);
extern int get_bound_dgram_socket_by_range(uint16_t start, uint16_t end, uint16_t *port, int *socketfd);
extern int send_string_to_ip_port(char *ip, uint16_t port, char *string, int socketfd);
extern int send_string_reply(const struct sockaddr *addr, char *string, int socketfd);
extern uint16_t port_from_sockaddr(const struct sockaddr *addr);

#endif
