#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TEST_PORT 3030

void testTCPFragmentation(void)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(TEST_PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    assert(connect(sock, (struct sockaddr *)&server,
                   sizeof(server)) == 0);

    // ส่งคำสั่งช่วงแรก
    const char *part1 = "Add_Board,My";
    assert(send(sock, part1, strlen(part1), 0)
           == (ssize_t)strlen(part1));

    // เว้นช่วงเพื่อให้ Server มีโอกาส recv() ก่อน
    sleep(1);

    // ส่งคำสั่งช่วงที่สอง
    const char *part2 = " Project\n";
    assert(send(sock, part2, strlen(part2), 0)
           == (ssize_t)strlen(part2));

    char response[1024];
    int n = recv(sock, response, sizeof(response) - 1, 0);
    assert(n > 0);

    response[n] = '\0';
    printf("Server response: %s\n", response);

    close(sock);
}

int main(void)
{
    testTCPFragmentation();
    return 0;
}