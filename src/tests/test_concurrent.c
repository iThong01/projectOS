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
/* ส่ง Request ให้ครบทุก Byte */
int sendTestRequest(int sock, const char *request)
{
    size_t length = strlen(request);
    size_t sent = 0;

    while (sent < length)
    {
        ssize_t n = send(sock, request + sent,
                         length - sent, MSG_NOSIGNAL);

        if (n <= 0)
            return -1;

        sent += (size_t)n;
    }

    return 0;
}

/* รับ Response ให้ครบหนึ่งบรรทัด */
int readTestLine(int sock, char *buffer, size_t size)
{
    size_t total = 0;

    if (size < 2)
        return -1;

    while (total < size - 1)
    {
        ssize_t n = recv(sock, buffer + total, 1, 0);

        if (n <= 0)
            return -1;

        total += (size_t)n;

        if (buffer[total - 1] == '\n')
        {
            buffer[total] = '\0';
            return 0;
        }
    }

    return -1;
}

/* ส่งคำสั่งที่คาดว่าจะสำเร็จ */
void expectTestSuccess(const char *request,
                       const char *expected)
{
    int sock = connectTestClient();
    char response[128];

    assert(sendTestRequest(sock, request) == 0);
    assert(readTestLine(sock, response,
                        sizeof(response)) == 0);

    if (strcmp(response, expected) != 0)
    {
        fprintf(stderr, "Expected: %sGot: %s",
                expected, response);
        assert(0);
    }

    close(sock);
}

/* ค้นหา Board ID จากชื่อ Board */
int findTestBoardId(const char *title)
{
    int sock = connectTestClient();
    char line[1024];
    int status, count;
    int foundId = -1;

    assert(sendTestRequest(sock, "Get_Boards\n") == 0);
    assert(readTestLine(sock, line, sizeof(line)) == 0);

    assert(sscanf(line, "Get_Boards,%d,%d",
                  &status, &count) == 2);
    assert(status == 0);

    for (int i = 0; i < count; i++)
    {
        assert(readTestLine(sock, line, sizeof(line)) == 0);

        char *comma = strchr(line, ',');
        assert(comma != NULL);

        *comma = '\0';

        char *boardTitle = comma + 1;
        boardTitle[strcspn(boardTitle, "\r\n")] = '\0';

        if (strcmp(boardTitle, title) == 0)
            foundId = atoi(line);
    }

    close(sock);
    return foundId;
}

int findTestTaskId(int boardId, const char *title)
{
    int sock = connectTestClient();
    char request[128];
    char line[1024];
    int status, count;
    int foundId = -1;

    snprintf(request, sizeof(request),
             "Get_Task,%d\n", boardId);

    assert(sendTestRequest(sock, request) == 0);
    assert(readTestLine(sock, line, sizeof(line)) == 0);

    assert(sscanf(line, "Get_Task,%d,%d",
                  &status, &count) == 2);
    assert(status == 0);

    for (int i = 0; i < count; i++)
    {
        assert(readTestLine(sock, line, sizeof(line)) == 0);

        char *firstComma = strchr(line, ',');
        char *lastComma = strrchr(line, ',');

        assert(firstComma != NULL);
        assert(lastComma != NULL);
        assert(firstComma != lastComma);

        *firstComma = '\0';
        *lastComma = '\0';

        char *taskTitle = firstComma + 1;

        if (strcmp(taskTitle, title) == 0)
            foundId = atoi(line);
    }

    close(sock);
    return foundId;
}

void testMixedCRUD()
{
    char request[256];
    char boardTitle[128];

    /*
     * STEP 1: สร้าง Board สำหรับ Test
     */
    snprintf(boardTitle, sizeof(boardTitle),
             "IT07 Board %ld %ld",
             (long)getpid(), (long)time(NULL));

    snprintf(request, sizeof(request),
             "Add_Board,%s\n", boardTitle);

    expectTestSuccess(request, "Add_Board,0\n");

    int boardId = findTestBoardId(boardTitle);
    assert(boardId >= 0);

    /*
     * STEP 2: สร้าง Task สำหรับ Update และ Delete
     */
    snprintf(request, sizeof(request),
             "Add_Task,%d,IT07 Update Target\n",
             boardId);
    expectTestSuccess(request, "Add_Task,0\n");

    snprintf(request, sizeof(request),
             "Add_Task,%d,IT07 Delete Target\n",
             boardId);
    expectTestSuccess(request, "Add_Task,0\n");

    int updateTaskId =
        findTestTaskId(boardId, "IT07 Update Target");

    int deleteTaskId =
        findTestTaskId(boardId, "IT07 Delete Target");

    assert(updateTaskId >= 0);
    assert(deleteTaskId >= 0);
    assert(updateTaskId != deleteTaskId);

    printf("Test Board ID: %d\n", boardId);
    printf("Update Task ID: %d\n", updateTaskId);
    printf("Delete Task ID: %d\n", deleteTaskId);

    /*
     * STEP 3: เตรียม Concurrent Clients
     */
    pid_t pids[MIXED_CLIENT_COUNT];
    int startPipe[2];

    assert(pipe(startPipe) == 0);

    for (int i = 0; i < MIXED_CLIENT_COUNT; i++)
    {
        pid_t pid = fork();
        assert(pid >= 0);

        if (pid == 0)
        {
            /* Child Process */
            close(startPipe[1]);

            char signal;

            if (read(startPipe[0], &signal, 1) != 1)
                _exit(1);

            close(startPipe[0]);

            int sock = connectTestClient();

            char childRequest[256];
            const char *expectedResponse;

            /*
             * Client 1: Add Task
             * Client 2: Update Task
             * Client 3: Delete Task
             */
            if (i == 0)
            {
                snprintf(childRequest,
                         sizeof(childRequest),
                         "Add_Task,%d,Mixed New Task\n",
                         boardId);

                expectedResponse = "Add_Task,0\n";
            }
            else if (i == 1)
            {
                snprintf(childRequest,
                         sizeof(childRequest),
                         "Update_Task,%d,%d,Mixed Updated Task\n",
                         boardId, updateTaskId);

                expectedResponse = "Update_Task,0\n";
            }
            else
            {
                snprintf(childRequest,
                         sizeof(childRequest),
                         "Delete_Task,%d,%d\n",
                         boardId, deleteTaskId);

                expectedResponse = "Delete_Task,0\n";
            }

            /*
             * STEP 4: ส่ง Request และรับ Response
             */
            if (sendTestRequest(sock, childRequest) != 0)
            {
                close(sock);
                _exit(1);
            }

            char response[128];

            if (readTestLine(sock, response,
                             sizeof(response)) != 0)
            {
                close(sock);
                _exit(1);
            }

            close(sock);

            if (strcmp(response, expectedResponse) != 0)
            {
                fprintf(stderr,
                        "Client %d: expected %s, got %s\n",
                        i + 1,
                        expectedResponse,
                        response);
                _exit(1);
            }

            _exit(0);
        }

        /* Parent เก็บ PID */
        pids[i] = pid;
    }

    /*
     * STEP 5: ปล่อยให้ Child เริ่มพร้อมกัน
     */
    close(startPipe[0]);

    for (int i = 0; i < MIXED_CLIENT_COUNT; i++)
    {
        char signal = 'S';

        assert(write(startPipe[1],
                     &signal, 1) == 1);
    }

    close(startPipe[1]);

    int passed = 0;

    for (int i = 0; i < MIXED_CLIENT_COUNT; i++)
    {
        int status;

        assert(waitpid(pids[i],
                       &status, 0) == pids[i]);

        if (WIFEXITED(status) &&
            WEXITSTATUS(status) == 0)
        {
            passed++;
        }
    }

    printf("Successful clients: %d/%d\n",
           passed, MIXED_CLIENT_COUNT);

    assert(passed == MIXED_CLIENT_COUNT);

    /*
     * STEP 7: ตรวจสอบข้อมูลหลัง Concurrent CRUD
     */
    int addedTaskId =
        findTestTaskId(boardId, "Mixed New Task");

    int updatedTaskId =
        findTestTaskId(boardId, "Mixed Updated Task");

    int deletedTaskId =
        findTestTaskId(boardId, "IT07 Delete Target");

    assert(addedTaskId >= 0);
    assert(updatedTaskId == updateTaskId);
    assert(deletedTaskId == -1);

    printf("[PASS] IT07 Concurrent Task Mixed CRUD\n");
}

int main()
{
    // testConcurrentClients();
    testMixedCRUD();
    return 0;
}