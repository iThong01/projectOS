#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(3030);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    assert(connect(sock, (struct sockaddr *)&server,sizeof(server)) == 0);

    const char *commands ="Add_Board,Test A\n""Add_Board,Test B\n";
    assert(send(sock, commands, strlen(commands), 0)== (ssize_t)strlen(commands));

    char response[1024];
    size_t total = 0;
    int responseCount = 0;

    while (responseCount < 2){
        ssize_t n = recv(sock, response + total,sizeof(response) - total - 1, 0);
        assert(n > 0);
        total += n;
        response[total] = '\0';

        responseCount = 0;
        for (size_t i = 0; i < total; i++)
        {
            if (response[i] == '\n')
                responseCount++;
        }
    }

    printf("Server responses:\n%s", response);
    assert(responseCount == 2);
    printf("[PASS] IT03 Multiple Commands\n");

    close(sock);
    return 0;
}