EXECUTABLE	= ../parallel_wrapper
LIB		= -L../lib
INCLUDE		= -I../include
SOURCE 		= network.c log.c main.c parse_args.c
DETAIL		= -DDETAIL
CFLAGS		= -g -O2 -Wall -Werror ${INCLUDE} ${DETAIL}

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
