#include "wrapper.h"
#include "string_util.h"

/* STAT */
#include <sys/types.h>
#include <sys/stat.h>

#include <setjmp.h>
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

int jmpset = 0;
sigjmp_buf jmpbuf;

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
		sigsetjmp(jmpbuf, 1); /* NOTE: This line must be right before we check exit_flag */
		jmpset = 1;
		if (exit_flag)
		{
			cleanup(par_wrapper, 250);
		}
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
			/* NOTE: This must be set before calling receive from */
			message -> len = sizeof(struct sockaddr_storage);
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
				print(PRNT_WARN, "Failed to handle command %d. RC = %d\n", command, RC);
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
	int RC, rank;
	/* CMD <RANK> */
	parallel_wrapper *par_wrapper = message -> par_wrapper;	
	/* Make sure the format is correct */
	if (message -> args -> dim != 2)
	{
		print(PRNT_WARN, "Invalid ACK packet. Expected '<ACK>:<RANK>'\n");
		return 1;
	}
	RC = parse_integer(message -> args -> strings[1], &rank);
	if (RC != 0)
	{
		print(PRNT_WARN, "Failed to parse rank\n");
		return 2;
	}
	if (rank < 0 || rank >= par_wrapper -> num_procs)
	{
		print(PRNT_WARN, "Invalid rank (%d)\n", rank);
		return 3;
	}
	if (par_wrapper -> this_machine -> rank != MASTER && rank != 0)
	{
		print(PRNT_WARN, "Unable to receive ACK from non-MASTER rank\n");
		return 3;
	}
	if (par_wrapper -> this_machine -> rank != MASTER && 
			(par_wrapper -> master == NULL || par_wrapper -> master -> ip_addr == NULL))
	{
		print(PRNT_WARN, "MASTER not intialized yet\n");
		return 4;
	}	

	/* Make sure that the source matches the registered machine */
	char *ip_addr = (char *)malloc(INET6_ADDRSTRLEN * sizeof(char));
	if (ip_addr == (char *)NULL)
	{
		print(PRNT_WARN, "Unable to allocate space for an IP address string\n");
		return 3;
	}
	RC = ip_str_from_sockaddr((struct sockaddr *)&message -> from,
		   ip_addr, INET6_ADDRSTRLEN);
	if (RC != 0)
	{
		free(ip_addr);
		print(PRNT_WARN, "Unable to obtain source of ACK signal\n");
		return 4;
	}	
	uint16_t port = port_from_sockaddr((struct sockaddr *)&message -> from);
	if (par_wrapper -> this_machine -> rank != MASTER)
	{
		/* Check for correct source */
		if (strcmp(message -> par_wrapper -> master -> ip_addr, ip_addr) != 0 ||
			message -> par_wrapper -> master -> port != port)
		{
			print(PRNT_WARN, "ACK from MASTER (%s:%d) does not match IP address of source (%s:%d)\n", 
				rank, par_wrapper -> master -> ip_addr, 
				par_wrapper -> master -> port, ip_addr, port);
			free(ip_addr);
			return 4;
		}
		free(ip_addr);
		pthread_mutex_lock(&par_wrapper -> mutex);
		gettimeofday(&par_wrapper -> master -> last_alive, NULL);
		pthread_mutex_unlock(&par_wrapper -> mutex);
		return 0;
	}
	else if (par_wrapper -> machines[rank] == (machine *)NULL)
	{
		print(PRNT_WARN, "Cannot receive an ACK from rank %d. It has not registered yet\n", rank);
		free(ip_addr);
		return 4;
	}
	
	if (strcmp(message -> par_wrapper -> machines[rank] -> ip_addr, ip_addr) != 0 ||
			message -> par_wrapper -> machines[rank] -> port != port)
	{
		print(PRNT_WARN, "Registered rank %d (%s:%d) does not match IP address of source (%s:%d)\n", 
				rank, ip_addr, port, 
				par_wrapper -> machines[rank] -> ip_addr, par_wrapper -> machines[rank] -> port);
	  	free(ip_addr);
	   	return 5;	
	}
	free(ip_addr);
	/* Update the last seen from time */
	pthread_mutex_lock(&par_wrapper -> mutex);
	gettimeofday(&par_wrapper -> machines[rank] -> last_alive, NULL);
	pthread_mutex_unlock(&par_wrapper -> mutex);
	return 0;
}

static int handle_query(struct udp_message *message)
{
	/* Response with an ACK */
	if (message -> args -> dim != 1)
	{
		print(PRNT_WARN, "Invalid QUERY packet. Expected <QUERY>\n");
		return 1;	
	}

	return ack(message -> par_wrapper -> command_socket, 
			message -> par_wrapper -> this_machine -> rank, 
			(struct sockaddr *)&message -> from); 
}

static int handle_term(struct udp_message *message)
{
	int RC, return_code;
	uint16_t port;
	if (message -> args -> dim != 2)
	{
		print(PRNT_WARN, "Invalid TERM packet. Expected <TERM>:<RC>\n");
		return 1;
	}
	/* 1) If I am MASTER, I am not allowed to get a term signal */
	if (message -> par_wrapper -> this_machine -> rank == MASTER)
	{
		print(PRNT_WARN, "MASTER does not accept TERM packets\n");
		return 1;
	}
	/* make sure that the master is initialized */
	if (message -> par_wrapper -> master == NULL || message -> par_wrapper -> master -> ip_addr == NULL)
	{
		print(PRNT_WARN, "MASTER process not yet initialized\n");
		return 1;
	}
	
	RC = parse_integer(message -> args -> strings[1], &return_code);
	if (RC != 0)
	{
		print(PRNT_WARN, "Failed to parse return code\n");
		return 2;
	}

	/* 2) The TERM signal must come from the master's command port */
	char *ip_addr = (char *)malloc(INET6_ADDRSTRLEN * sizeof(char));
	if (ip_addr == (char *)NULL)
	{
		print(PRNT_WARN, "Unable to allocate space for an IP address string\n");
		return 3;
	}
	RC = ip_str_from_sockaddr((struct sockaddr *)&message -> from,
		   ip_addr, INET6_ADDRSTRLEN);
	if (RC != 0)
	{
		free(ip_addr);
		print(PRNT_WARN, "Unable to obtain source of TERM signal\n");
		return 4;
	}	
	port = port_from_sockaddr((struct sockaddr *)&message -> from);
	if (strcmp(message -> par_wrapper -> master -> ip_addr, ip_addr) == 0 &&
			message -> par_wrapper -> master -> port == port)
	{
		RC = ack(message -> par_wrapper -> command_socket, 
			message -> par_wrapper -> this_machine -> rank, 
			(struct sockaddr *)&message -> from); 
		if (RC != 0)
		{
			print(PRNT_WARN, "Unable to send ACK for TERM\n");
		}

		/* Term signal is valid */
		print(PRNT_INFO, "Received valid term signal from master. Exitting.\n");
		free(ip_addr);
		cleanup(message -> par_wrapper, return_code);
	}
	print(PRNT_WARN, "Source of TERM signal was not MASTER\n");
	free(ip_addr);
	return 5;
}

static int handle_create_link(struct udp_message *message)
{
	/* CREATE_LINK <SRC> <DEST> */
	int RC;
	if (message -> args -> dim != 3)
	{
		print(PRNT_WARN, "Invalid CREATE_LINK packet. Expected <CREATE_LINK>:<SRC>:<DEST>\n");
		return 1;
	}
	/* Make sure that the symlinks list is initialized */
	if (!is_valid_sll(message -> par_wrapper -> symlinks))
	{
		print(PRNT_WARN, "SLL symlinks list is not initialized\n");
		return 2;
	}

	remove_quotes(message -> args -> strings[1]);
	remove_quotes(message -> args -> strings[2]);
	trim(message -> args -> strings[1]);
	trim(message -> args -> strings[2]);

	/* Make sure that the source exists */
	struct stat st;
	if (stat(message -> args -> strings[1], &st) != 0)
	{
		print(PRNT_WARN, "Unable to create softlink from %s -> %s. Source does not exist.\n",
				message -> args -> strings[1], message -> args -> strings[2]);
		return 3;
	}

	/* Attempt to create the softlink */
	if (symlink(message -> args -> strings[1], message -> args -> strings[2]) != 0)
	{
		print(PRNT_WARN, "Unable to create symlink from %s -> %s\n", 
				message -> args -> strings[1], message -> args -> strings[2]);
		return 4;
	}

	/* Save this information for later */
	char *dest = strdup(message -> args -> strings[2]);
	pthread_mutex_lock(&message -> par_wrapper -> mutex);
	sll_add_element(message -> par_wrapper -> symlinks, (void *)dest);
	pthread_mutex_unlock(&message -> par_wrapper -> mutex);
	
	/* Send ACK back */
	RC = ack(message -> par_wrapper -> command_socket, 
		message -> par_wrapper -> this_machine -> rank, 
		(struct sockaddr *)&message -> from); 
	if (RC != 0)
	{
		print(PRNT_WARN, "Unable to send ACK for CREATE_LINK\n");
	}
	return 0;
}

static int handle_send_file(struct udp_message *message)
{
	print(PRNT_INFO, "NOT IMPLEMENTED -- SEND_FILE Handler\n");
	return 0;
}

static int handle_register(struct udp_message *message)
{
	/* <REGISTER>:<RANK>:<IWD> */
	int RC, rank;
	if (message -> args -> dim != 3)
	{
		print(PRNT_WARN, "Invalid CREATE_LINK packet. Expected <REGISTER>:<RANK>:<IWD>\n");
		return 1;
	}
	/* Only the MASTER is allowed to register ranks */
	if (message -> par_wrapper -> this_machine -> rank != MASTER)
	{
		print(PRNT_WARN, "Only the MASTER is allowed to register individuals\n");
		return 2;
	}

	/* Attempt to parse the rank */
	RC = parse_integer(message -> args -> strings[1], &rank);
	if (RC != 0)
	{
		print(PRNT_WARN, "Failed to parse rank\n");
		return 3;
	}
	if (rank < 0 || rank >= message -> par_wrapper -> num_procs)
	{
		print(PRNT_WARN, "Invalid rank (%d)\n", rank);
		return 4;
	}

	/* Trim and remove quotes on the IWD */
	remove_quotes(message -> args -> strings[2]);
	trim(message -> args -> strings[2]);

	/* Get the IP Address and port of this host */
	char *ip_addr = (char *)malloc(INET6_ADDRSTRLEN * sizeof(char));
	if (ip_addr == (char *)NULL)
	{
		print(PRNT_WARN, "Unable to allocate space for an IP address string\n");
		return 3;
	}
	RC = ip_str_from_sockaddr((struct sockaddr *)&message -> from,
		   ip_addr, INET6_ADDRSTRLEN);
	if (RC != 0)
	{
		free(ip_addr);
		print(PRNT_WARN, "Unable to obtain source of TERM signal\n");
		return 4;
	}	
	uint16_t port = port_from_sockaddr((struct sockaddr *)&message -> from);
	
	/**
	 * Check if this machine has already been registered
	 */
	if (message -> par_wrapper -> machines[rank] == NULL)
	{
		message -> par_wrapper -> machines[rank] = (machine *) calloc(1, sizeof(struct machine));
		if (message -> par_wrapper -> machines[rank] == (machine *)NULL)
		{
			print(PRNT_WARN, "Unable to allocate space for new machine\n");
			free(ip_addr);
			return 5;
		}
		message -> par_wrapper -> machines[rank] -> ip_addr = strdup(ip_addr);
		message -> par_wrapper -> machines[rank] -> rank = rank;
		message -> par_wrapper -> machines[rank] -> iwd = strdup(message -> args -> strings[2]);
		message -> par_wrapper -> machines[rank] -> port = port;	
	}
	else
	{
		/** 
		 * This machine has already been allocated -
		 * check if this is the same machine
		 */
		if (strcmp(message -> par_wrapper -> machines[rank] -> ip_addr, ip_addr) != 0 ||
			message -> par_wrapper -> machines[rank] -> port != port)
		{
			print(PRNT_WARN, "Command REGISTER from rank %d (%s:%d) does not originate from already registered address (%s:%d)\n",
				rank, ip_addr, port, message -> par_wrapper -> machines[rank] -> ip_addr, 
				message -> par_wrapper -> machines[rank] -> port);
			free(ip_addr);
			return 6;
		}
		print(PRNT_INFO, "Received command REGISTER from rank %d, but already registered. Sending ACK\n", rank);
	}

	/* Send ACK back */
	RC = ack(message -> par_wrapper -> command_socket, 
		message -> par_wrapper -> this_machine -> rank, 
		(struct sockaddr *)&message -> from); 
	if (RC != 0)
	{
		print(PRNT_WARN, "Unable to send ACK for REGISTER\n");
	}
	free(ip_addr);
	return 0;
}
