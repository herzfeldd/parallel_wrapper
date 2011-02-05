EXECUTABLE	= ../parallel_wrapper
INCLUDE		= -I../include
SOURCE 		= network.c log.c
DETAIL		= -DDETAIL
CFLAGS		= -g -O2 -Wall -Werror ${INCLUDE} ${DETAIL}

# Define the source file locations
SRC		= ${SOURCE:%.c=../src/%.c}
OBJ		= ${SRC:%.c=%.o}

${EXECUTABLE}: ${OBJ}
	${CC} -o $@ ${LDFLAGS} ${OBJ} ${LDLIBS} -lpthread

clean:
	rm -rf *.o ${OBJ} ${EXECUTABLE}
