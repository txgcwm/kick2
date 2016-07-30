
##############        kick2 module makefile     ###############

REFERENCE_DIR        = ./reference

CC = g++

CFLAGS = -pipe  -O0 -W -Wall -Wpointer-arith -Wno-unused-parameter -Werror -g -lpthread -lrt

ALL_INC = -I ./include \
          -I $(REFERENCE_DIR)

ALL_OBJ = ./http_parser.o \
          ./http_parser_ext.o \
		  ./zmalloc.o \
		  ./ae.o \
		  ./thread.o \
		  ./datetime.o \
		  ./ae_engine.o \
		  ./socket.o \
		  ./hls_handler.o \
		  ./http_helper.o \
		  ./http_client.o \
		  ./string_helper.o \
		  ./kick2.o \
          ./main.o

all: kick2
kick2: ${ALL_OBJ}
	${CC} -o kick2 ${CFLAGS} $(ALL_OBJ)
	rm -f $(ALL_OBJ)

http_parser.o:                       ./reference/http_parser.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./reference/http_parser.c

http_parser_ext.o:                   ./include/http_parser_ext.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/http_parser_ext.cpp

zmalloc.o:                           ./reference/zmalloc.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./reference/zmalloc.c

ae.o:                                ./reference/ae.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./reference/ae.c

thread.o:                            ./include/thread.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/thread.cpp

datetime.o:                          ./include/datetime.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/datetime.cpp

ae_engine.o:                         ./include/ae_engine.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/ae_engine.cpp

socket.o:                            ./include/socket.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/socket.cpp

hls_handler.o:                       ./include/hls_handler.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/hls_handler.cpp

http_helper.o:                       ./include/http_helper.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/http_helper.cpp

http_client.o:                       ./include/http_client.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/http_client.cpp

string_helper.o:                     ./include/string_helper.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/string_helper.cpp

kick2.o:                             ./include/kick2.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/kick2.cpp

main.o:
	${CC} -c ${CFLAGS} ${ALL_INC}    ./src/main.cpp

clean: 
	rm -f ${ALL_OBJ}    ./kick2
