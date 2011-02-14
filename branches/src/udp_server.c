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
struct udp_handler
{
	CMD command;
	int (*handler)(struct udp_message *); /**< The handler */
#if 0
	int (*query_handler)(struct udp_message *); /**< Handler for QUERY */	
	int (*ack_handler)(struct udp_message *); /**< Handler for ACK */
	int (*term_handler)(struct udp_message *); /**< Handler for TERM */
	int (*create_link_handler)(struct udp_message *); /**< Create link handler */
	int (*send_file_handler)(struct udp_message *); /**< Send file */
	int (*register_handler)(struct udp_message *); /**< Register */
#endif
};

#define BUFFER_SIZE (1024u)

/* Local Function Prototypes */
static void *process_message(void *ptr);
static int handle_ack(struct udp_message *message);
static int handle_query(struct udp_message *message);
static int handle_term(struct udp_message *message);
static int handle_create_link(struct udp_message *message);
static int handle_send_file(struct udp_message *message);
static int handle_register(struct udp_message *message);

/**
 * Instance of udp_handlers defining the various handler functions
 */
static struct udp_handler handlers[] = 
{
	{CMD_QUERY, handle_query},
	{CMD_ACK, handle_ack},
	{CMD_TERM, handle_term},
	{CMD_CREATE_LINK, handle_create_link},
	{CMD_SEND_FILE, handle_send_file},
	{CMD_REGISTER, handle_register},
	{CMD_NULL, NULL}
} ;


	
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


/**
 * Thread Entry Point: Process and new message
 *
 * Processes a message received on the command port. A message
 * structure is passed via the incoming pointer. This message 
 * structure is freed by THIS function before returning.
 *
 * @param ptr Pointer to the message structure
 * @return NULL
 */
static void *process_message(void *ptr)
{
	int RC;
	CMD command;
	int temp;
	struct udp_message *message = (struct udp_message *)ptr;
	if (message == (struct udp_message *)NULL)
	{
		print(PRNT_WARN, "Null message passed to message handler\n");
		return NULL;
	}
	if (! is_valid_strarray(message -> args))
	{
		free_strarray(message -> args);
		free(message);
		print(PRNT_WARN, "Null message arguments passed to message handler\n");
		return NULL;
	}
	
	RC = parse_integer(message -> args -> strings[0], &temp);
	if (RC != 0)
	{
		free_strarray(message -> args);
		free(message);
		print(PRNT_WARN, "Unable to parse message. Invalid command.\n");
		return NULL;
	}
	command = (CMD) temp;

	/* Determine the handler for this command */
	temp = 0;
	while ((handlers[temp].handler != NULL) && handlers[temp].command != CMD_NULL)
	{
		if (command == handlers[temp].command)
		{
			/* Call the handler */
			RC = handlers[temp].handler(message);
			if (RC != 0)
			{
				print(PRNT_WARN, "Failed to handle command %d\n", command);
			}
			free_strarray(message -> args);
			free(message);
			return NULL;
		}
		temp++; /* Move on to next handler */
	}	

	print(PRNT_WARN, "No handler for command %d or unrecognized command\n", command);
	/* Clean up */
	free_strarray(message -> args);
	free(message);
	return NULL;
}

/*--------------------------------------------------------------------------*/
/*  MESSAGE HANDLERS:                                                       */
/*--------------------------------------------------------------------------*/

static int handle_ack(struct udp_message *message)
{
	print(PRNT_INFO, "ACK Handler\n");
	return 0;
}

static int handle_query(struct udp_message *message)
{
	print(PRNT_INFO, "QUERY Handler\n");
	return 0;
}

static int handle_term(struct udp_message *message)
{
	print(PRNT_INFO, "TERM Handler\n");
	return 0;
}

static int handle_create_link(struct udp_message *message)
{
	print(PRNT_INFO, "CREATE_LINK Handler\n");
	return 0;
}

static int handle_send_file(struct udp_message *message)
{
	print(PRNT_INFO, "SEND_FILE Handler\n");
	return 0;
}

static int handle_register(struct udp_message *message)
{
	print(PRNT_INFO, "REGISTER Handler\n");
	return 0;
}
