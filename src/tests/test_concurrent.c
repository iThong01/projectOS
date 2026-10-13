#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#define TEST_PORT 3030
#define CLIENT_COUNT 5
#define TEST_BOARD_ID 28
#define MIXED_CLIENT_COUNT 3

int connectTestClient()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    struct sockaddr_in server = {0};

    server.sin_family = AF_INET;
    server.sin_port = htons(TEST_PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    assert(connect(sock,
                   (struct sockaddr *)&server,
                   sizeof(server)) == 0);

    return sock;
}
void testConcurrentClients()
{
    pid_t pids[CLIENT_COUNT];

    // ใช้ Pipe เพื่อให้ Child รอเริ่มพร้อมกัน
    int startPipe[2];
    assert(pipe(startPipe) == 0);

    for (int i = 0; i < CLIENT_COUNT; i++)
    {
        pid_t pid = fork();
        assert(pid >= 0);

        if (pid == 0)
        {
            // Child Process
            close(startPipe[1]);

            char signal;
            ssize_t n = read(startPipe[0], &signal, 1);
            if (n != 1)
                _exit(1);

            close(startPipe[0]);

            int sock = connectTestClient();

            char request[128];
            snprintf(request, sizeof(request),
                     "Add_Board,Concurrent Board %d\n", i + 1);

            size_t length = strlen(request);
            size_t sent = 0;

            while (sent < length)
            {
                ssize_t count = send(
                    sock,
                    request + sent,
                    length - sent,
                    MSG_NOSIGNAL
                );

                if (count <= 0)
                {
                    close(sock);
                    _exit(1);
                }

                sent += (size_t)count;
            }

            // รับ Response จนครบหนึ่งบรรทัด
            char response[128];
            size_t total = 0;

            while (total < sizeof(response) - 1)
            {
                ssize_t count = recv(
                    sock,
                    response + total,
                    1,
                    0
                );

                if (count <= 0)
                {
                    close(sock);
                    _exit(1);
                }

                total += (size_t)count;

                if (response[total - 1] == '\n')
                    break;
            }

            response[total] = '\0';

            close(sock);

            if (strcmp(response, "Add_Board,0\n") != 0)
                _exit(1);

            _exit(0);
        }

        // Parent เก็บ PID ของ Child
        pids[i] = pid;
    }

    close(startPipe[0]);

    // ปล่อยให้ Client ทั้ง 5 ตัวเริ่มทำงาน
    for (int i = 0; i < CLIENT_COUNT; i++)
    {
        char signal = 'S';
        assert(write(startPipe[1], &signal, 1) == 1);
    }

    close(startPipe[1]);

    int passed = 0;

    for (int i = 0; i < CLIENT_COUNT; i++)
    {
        int status;
        assert(waitpid(pids[i], &status, 0) == pids[i]);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            passed++;
    }

    printf("Successful clients: %d/%d\n", passed, CLIENT_COUNT);

    assert(passed == CLIENT_COUNT);

    printf("[PASS] IT05 Concurrent Clients\n");
}

void testMixedCRUD()
{
    pid_t pids[MIXED_CLIENT_COUNT];
    int startPipe[2];

    assert(pipe(startPipe) == 0);

    for (int i = 0; i < MIXED_CLIENT_COUNT; i++)
    {
        pid_t pid = fork();
        assert(pid >= 0);

        if (pid == 0)
        {
            close(startPipe[1]);

            char signal;
            if (read(startPipe[0], &signal, 1) != 1)
                _exit(1);

            close(startPipe[0]);

            int sock = connectTestClient();

            char request[256];
            const char *expectedResponse;

            if (i == 0)
            {
                snprintf(request, sizeof(request),
                         "Add_Task,28,Mixed New Task\n");

                expectedResponse = "Add_Task,0\n";
            }
            else if (i == 1)
            {
                snprintf(request, sizeof(request),
                         "Update_Task,28,1,Mixed Updated Task\n");

                expectedResponse = "Update_Task,0\n";
            }
            else
            {
                snprintf(request, sizeof(request),
                         "Delete_Task,28,2\n");

                expectedResponse = "Delete_Task,0\n";
            }

            size_t length = strlen(request);
            size_t sent = 0;

            while (sent < length)
            {
                ssize_t count = send(
                    sock,
                    request + sent,
                    length - sent,
                    MSG_NOSIGNAL
                );

                if (count <= 0)
                {
                    close(sock);
                    _exit(1);
                }

                sent += (size_t)count;
            }

            char response[128];
            size_t total = 0;

            while (total < sizeof(response) - 1)
            {
                ssize_t count = recv(
                    sock,
                    response + total,
                    1,
                    0
                );

                if (count <= 0)
                {
                    close(sock);
                    _exit(1);
                }

                total += (size_t)count;

                if (response[total - 1] == '\n')
                    break;
            }

            response[total] = '\0';

            close(sock);

            if (strcmp(response, expectedResponse) != 0)
            {
                fprintf(stderr,
                        "Client %d: expected %s, got %s\n",
                        i + 1, expectedResponse, response);
                _exit(1);
            }

            _exit(0);
        }

        pids[i] = pid;
    }

    close(startPipe[0]);

    for (int i = 0; i < MIXED_CLIENT_COUNT; i++)
    {
        char signal = 'S';
        assert(write(startPipe[1], &signal, 1) == 1);
    }

    close(startPipe[1]);

    int passed = 0;

    for (int i = 0; i < MIXED_CLIENT_COUNT; i++)
    {
        int status;

        assert(waitpid(pids[i], &status, 0) == pids[i]);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            passed++;
    }

    printf("Successful clients: %d/%d\n",
           passed, MIXED_CLIENT_COUNT);

    assert(passed == MIXED_CLIENT_COUNT);

    printf("[PASS] Concurrent Task Mixed CRUD\n");
}
int main()
{
    // testConcurrentClients();
    testMixedCRUD();
    return 0;
}