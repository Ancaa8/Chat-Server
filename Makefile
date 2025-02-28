CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

all: server subscriber

server: server.c common.o
	gcc $(CFLAGS) -o server server.c common.o

subscriber: subscriber.c common.o
	gcc $(CFLAGS) -o subscriber subscriber.c common.o

common.o: common.c
	gcc $(CFLAGS) -c common.c

.PHONY: clean run_server run_subscriber

run_server: server
	./server ${IP_SERVER} ${PORT} || true

run_subscriber: subscriber
	./subscriber ${IP_SERVER} ${PORT} || true

clean:
	rm -rf server subscriber *.o *.dSYM
