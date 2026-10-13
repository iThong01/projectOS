#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TEST_PORT 3030

int main(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(TEST_PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    assert(connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0);

    char oversized[1000];
    memset(oversized, 'A', sizeof(oversized));
    memcpy(oversized, "Add_Board,", strlen("Add_Board,"));

    oversized[sizeof(oversized) - 2] = '\n';
    oversized[sizeof(oversized) - 1] = '\0';

    size_t length = strlen(oversized);
    size_t sent = 0;

    while (sent < length){
        ssize_t n = send(sock, oversized + sent,length - sent, 0);
        assert(n > 0);
        sent += (size_t)n;
    }
    printf("Oversized command sent.\n");
    const char *valid = "Add_Board,Valid Board\n";
    sent = 0;
    length = strlen(valid);

    while (sent < length){
        ssize_t n = send(sock, valid + sent, length - sent, 0);
        assert(n > 0);
        sent += (size_t)n;
    }

    struct timeval timeout = {
        .tv_sec = 3,
        .tv_usec = 0
    };
    assert(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&timeout, sizeof(timeout)) == 0);

    char response[1024];
    ssize_t n = recv(sock, response, sizeof(response) - 1, 0);
    assert(n > 0);
    response[n] = '\0';

    printf("Server response: %s", response);
    assert(strcmp(response, "Add_Board,0\n") == 0);
    printf("[PASS] IT04 Oversized Command\n");

    close(sock);
    return 0;
}