
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 32

// Primeste date de pe connfd1 si trimite mesajul receptionat pe connfd2
int receive_and_send(int connfd1, int connfd2, size_t len) {
  int bytes_received;
  char buffer[len];

  // Primim exact len octeti de la connfd1
  bytes_received = recv_all(connfd1, buffer, len);
  // S-a inchis conexiunea
  if (bytes_received == 0) {
    return 0;
  }
  DIE(bytes_received < 0, "recv");

  // Trimitem mesajul catre connfd2
  int rc = send_all(connfd2, buffer, len);
  if (rc <= 0) {
    perror("send_all");
    return -1;
  }

  return bytes_received;
}

void run_chat_multi_server(int listenfd) {

    struct pollfd poll_fds[MAX_CONNECTIONS];
    int num_sockets = 1;
    int rc;

    struct chat_packet received_packet;

    // Setam socket-ul listenfd pentru ascultare
    rc = listen(listenfd, MAX_CONNECTIONS);
    DIE(rc < 0, "listen");

    // Adaugam noul file descriptor (socketul pe care se asculta conexiuni) in
    // multimea poll_fds
    poll_fds[0].fd = listenfd;
    poll_fds[0].events = POLLIN;

    while (1) {
        // Asteptam sa primim ceva pe unul dintre cei num_sockets socketi
        rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < num_sockets; i++) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == listenfd) {
                    // Am primit o cerere de conexiune pe socketul de listen, pe care
                    // o acceptam
                    struct sockaddr_in cli_addr;
                    socklen_t cli_len = sizeof(cli_addr);
                    const int newsockfd =
                        accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
                    DIE(newsockfd < 0, "accept");

                    // Adaugam noul socket intors de accept() la multimea descriptorilor
                    // de citire
                    poll_fds[num_sockets].fd = newsockfd;
                    poll_fds[num_sockets].events = POLLIN;
                    num_sockets++;

                    // Primim ID-ul clientului de la subscriber
                    struct chat_packet id_packet;
                    int id_rc = recv_all(newsockfd, &id_packet, sizeof(id_packet));
                    DIE(id_rc < 0, "recv");

                    // Afisam ID-ul clientului
                    printf("New client %s connected from %s:%d.\n", id_packet.message,
                           inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                } else {
                    // Am primit date pe unul din socketii de client, asa ca le receptionam
                    int rc = recv_all(poll_fds[i].fd, &received_packet,
                                      sizeof(received_packet));
                    DIE(rc < 0, "recv");

                    if (rc == 0) {
                        printf("Socket-ul client %d a inchis conexiunea\n", i);
                        close(poll_fds[i].fd);

                        // Scoatem din multimea de citire socketul inchis
                        for (int j = i; j < num_sockets - 1; j++) {
                            poll_fds[j] = poll_fds[j + 1];
                        }

                        num_sockets--;
                    } else {
                        printf("S-a primit de la clientul de pe socketul %d mesajul: %s\n",
                               poll_fds[i].fd, received_packet.message);
                        if (strcmp(received_packet.message, "exit") == 0) {
                             printf("Client Disconnected\n");
                         }
                        /* TODO 2.1: Trimite mesajul catre toti ceilalti clienti */
                        for (int j = 1; j < num_sockets; j++) {
                            if (poll_fds[j].fd != poll_fds[i].fd) {
                                int send_rc = send_all(poll_fds[j].fd, &received_packet, sizeof(received_packet));
                                if (send_rc <= 0) {
                                    printf("Nu s-a putut trimite mesajul către clientul de pe socketul %d\n", poll_fds[j].fd);
                                }
                            }
                        }              
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("\n Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // Parsăm port-ul ca un număr
    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // Obținem un socket TCP pentru receptionarea conexiunilor
    const int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listenfd < 0, "socket");

    // Completăm in serv_addr adresa serverului, familia de adrese si portul pentru conectare
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    // Facem adresa socket-ului reutilizabila, ca să nu primim eroare în caz ca rulăm de 2 ori rapid
    const int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

    // Asociem adresa serverului cu socketul creat folosind bind
    rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind");

    // TODO: Așteptăm conexiuni și gestionăm-le
    run_chat_multi_server(listenfd);

    // Închidem listenfd
    close(listenfd);

    return 0;
}
