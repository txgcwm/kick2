
##############        kick2 module makefile     ###############

REFERENCE_DIR        = ./reference

CC      = g++

CFLAGS  = -pipe  -O0 -W -Wall -Wpointer-arith -Wno-unused-parameter -Werror -g

LIBS    = -lpthread -lrt -laio

ALL_INC = -I ./include \
          -I $(REFERENCE_DIR)

ALL_OBJ = ./http_parser.o \
          ./http_parser_ext.o \
		  ./zmalloc.o \
		  ./mem_pool.o \
		  ./ae.o \
		  ./ae_engine.o \
		  ./thread.o \
		  ./datetime.o \
		  ./socket.o \
		  ./file.o \
		  ./hls_handler.o \
		  ./http_helper.o \
		  ./http_client.o \
		  ./network_helper.o \
		  ./multicast_client.o \
		  ./string_helper.o \
		  ./kick2.o

all: kick2
kick2: ${ALL_OBJ} main.o unit_test.o
	${CC} -o kick2 ${CFLAGS} $(ALL_OBJ) $(LIBS) main.o
	${CC} -o test_kick2 ${CFLAGS} $(ALL_OBJ) $(LIBS) unit_test.o
	rm -f $(ALL_OBJ)  main.o unit_test.o

http_parser.o:                       ./reference/http_parser.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./reference/http_parser.c

http_parser_ext.o:                   ./include/http_parser_ext.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/http_parser_ext.cpp

zmalloc.o:                           ./reference/zmalloc.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./reference/zmalloc.c

mem_pool.o:                          ./include/mem_pool.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/mem_pool.cpp

ae.o:                                ./reference/ae.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./reference/ae.c

ae_engine.o:                         ./include/ae_engine.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/ae_engine.cpp

thread.o:                            ./include/thread.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/thread.cpp

datetime.o:                          ./include/datetime.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/datetime.cpp

socket.o:                            ./include/socket.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/socket.cpp

file.o:                              ./include/file.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/file.cpp

hls_handler.o:                       ./include/hls_handler.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/hls_handler.cpp

http_helper.o:                       ./include/http_helper.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/http_helper.cpp

http_client.o:                       ./include/http_client.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/http_client.cpp

network_helper.o:                    ./include/network_helper.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/network_helper.cpp

multicast_client.o:                  ./include/multicast_client.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/multicast_client.cpp

string_helper.o:                     ./include/string_helper.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/string_helper.cpp

kick2.o:                             ./include/kick2.h
	$(CC) -c ${CFLAGS} ${ALL_INC}    ./src/kick2.cpp

main.o:
	${CC} -c ${CFLAGS} ${ALL_INC}    ./src/main.cpp

unit_test.o:
	${CC} -c ${CFLAGS} ${ALL_INC}    ./src/unit_test.cpp

clean: 
	rm -f ${ALL_OBJ}    ./kick2 ./test_kick2
