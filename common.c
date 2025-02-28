#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

int recv_all(int sockfd, void *buffer, size_t len) {
    size_t bytes_received = recv(sockfd, buffer, len, 0);
    if (bytes_received != len) {
        return -1; 
    }
    return bytes_received;
}

int send_all(int sockfd, void *buffer, size_t len) {
    size_t bytes_sent = send(sockfd, buffer, len, 0);
    if (bytes_sent != len) {
        return -1;
    }
    return bytes_sent;
}
