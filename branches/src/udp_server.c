#include "wrapper.h"
#include "string_util.h"
/**
 * Define a structure which represents a single UDP message
 */
struct udp_message
{
	parallel_wrapper *par_wrapper; /**< The parallel wrapper */
	strarray *args; /**< The arguments */
  	struct sockaddr_storage from; /**< The sockaddr associated with this message */
	socklen_t len; /**< The length of the sockaddr_storage element */	
};

/**
 * Define a structure which contains the udp handlers for
 * the various commands
 */
struct udp_handlers
{
	int (*query_handler)(struct udp_message *);	
};

#define BUFFER_SIZE (1024u)

static void *process_message(void *ptr);
/**
 * Starts a UDP server
 *
 * Starts a UDP server on a UDP port. The port is chosen within
 * the range specified in the parallel_wrapper
 *
 * @ptr a void pointer which contains a parallel_wrapper
 * @return Nothing
 */
void *udp_server(void *ptr)
{
	parallel_wrapper *par_wrapper = (parallel_wrapper *) ptr;
	if (par_wrapper == (parallel_wrapper *)NULL)
	{
		print(PRNT_ERR, "Invalid parallel_wrapper\n");
		return NULL;
	}

	int RC;
	pthread_attr_t attr;
	pthread_t thread;
	default_pthead_attr(&attr);
	fd_set readfds;

	char delim[] = {':', '|'};

	/* Create a small buffer to hold messages */
	char *buffer = (char *) malloc(sizeof(char) * BUFFER_SIZE);
	if (buffer == (char *)NULL)
	{
		print(PRNT_ERR, "Unable to allocate buffer for udp server\n");
		return NULL;
	}

	/* At this point, we already have socket and a port */
	int n = par_wrapper -> command_socket + 1;
	while ( 1 )
	{
		FD_ZERO(&readfds);
		FD_SET(par_wrapper -> command_socket, &readfds);
		memset(buffer, 0, sizeof(char) * BUFFER_SIZE);
		RC = select(n, &readfds, NULL, NULL, NULL);
		if (RC == -1)
		{
			print(PRNT_ERR, "Select statement failed\n");
			return NULL;
		}
		else if (RC > 0 && FD_ISSET(par_wrapper -> command_socket, &readfds))
		{
			/* Service the message on the command port */
			struct udp_message *message = (struct udp_message *)calloc(1, sizeof(struct udp_message));
			if (message == (struct udp_message *)NULL)
			{
				print(PRNT_ERR, "Unable to allocate space for message\n");
				return NULL;
			}
			/* Fill in the message structure as well as we can */
			message -> par_wrapper = par_wrapper;
			/* Receive the message */
			recvfrom(par_wrapper -> command_socket, buffer, BUFFER_SIZE, 0, 
				(struct sockaddr *)&message -> from, &message -> len);
			/* Parse the buffer into arguments using the delims */
			message -> args = strsplit(delim, buffer); 

			/* Create the thread */
			pthread_create(&thread, &attr, &process_message, message);
		}
		else
		{
			/* We timed out */
		}
	}	

	return NULL;
}

static void *process_message(void *ptr)
{
	print(PRNT_INFO, "processing message\n");
	struct udp_message *message = (struct udp_message *)ptr;
	
	/* Clean up */
	free_strarray(message -> args);
	free(message);
	return NULL;
}

