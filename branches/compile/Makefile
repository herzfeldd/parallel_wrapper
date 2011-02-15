EXECUTABLE	= ../parallel_wrapper
LIB		= -L../lib
INCLUDE		= -I../include
SOURCE 		= chirp_util.c log.c main.c network_util.c string_util.c \
		  threads.c timer.c parse_args_env.c udp_server.c \
		  udp_client.c chirp.c sll.c cleanup.c
DETAIL		= -DDETAIL
# Add -O2 here
CFLAGS		= -g -Wall -Werror ${INCLUDE} ${DETAIL}

# Chirp library
CHIRP_LIB	= -lchirp_client_x86-64
LDFLAGS		= ${LIB} ${CHIRP_LIB}

# Define the source file locations
SRC		= ${SOURCE:%.c=../src/%.c}
OBJ		= ${SRC:%.c=%.o}

${EXECUTABLE}: ${OBJ}
	${CC} -o $@ ${LDFLAGS} ${OBJ} ${LDLIBS} ${CHIRP_LIB} -lpthread

clean:
	rm -rf *.o ${OBJ} ${EXECUTABLE}

again: clean ${EXECUTABLE}
