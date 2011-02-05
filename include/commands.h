#ifndef COMMANDS_H
#define COMMANDS_H

typedef enum CMD
{
	CMD_TERM, /**< Terminate */
	CMD_QUERY, /**< Query status */
	CMD_ALIVE, /**< I am alive */
	CMD_SEND_FILE /**< Send a file to this TCP port */
} CMD;

#endif /* COMMAND_H */
