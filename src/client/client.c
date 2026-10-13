#define _GNU_SOURCE
#include "client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "protocol.h"

int serverConnected = 1;
int connectToServer(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        perror("socket");
        return -1;
    }

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(3030);

    if(inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) != 1){
        close(sock);
        return -1;
    }
    if(connect(sock, (struct sockaddr *)&server,sizeof(server)) < 0){
        perror("connect");
        close(sock);
        return -1;
    }
    return sock;
}

int sendRequest(int sock, const char *request){
    size_t length = strlen(request);
    size_t sent = 0;

    while (sent < length) {
        ssize_t n = send(sock, request + sent,length - sent,MSG_NOSIGNAL);
        // ssize_t n = send(sock, request + sent,length - sent, 0);
        if (n <= 0) {
            perror("send");
            serverConnected = 0;
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

int receiveResponse(int sock){
    char response[1024];
    char* save =nullptr;
    char* out;
    ssize_t n = recv(sock, response,sizeof(response) - 1, 0);

    // if(n <= 0){
    //     printf("No response from server.\n");
    //     return -1;
    // }
    if (n == 0) {
        printf( COLOR_RED "ERROR Server disconnected.\n" COLOR_RESET);
        serverConnected = 0;
        return -1;
    }
    if (n < 0) {
        perror(COLOR_RED "ERROR recv" COLOR_RESET);
        serverConnected = 0;
        return -1;
    }
    response[n] = '\0';
    printf("Server Response: %s", response);
    
    char *command = strtok_r(response, ",", &save);
    char *status = strtok_r(NULL, ",\n\r", &save);

    if (command == NULL || status == NULL) {
        printf("Invalid server response.\n");
        return -1;
    }
    int statusCode = atoi(status);

    switch (statusCode){
        case STATUS_OK:
            printf(COLOR_GREEN "%s Done\n" COLOR_RESET, command);
            break;
        case STATUS_BOARD_NOT_FOUND:
            printf(COLOR_RED"%s Failed: Board not found.\n"COLOR_RESET, command);
            break;
        case STATUS_FAIL:
            printf(COLOR_RED"%s Failed: Server operation failed.\n"COLOR_RESET, command);
            break;
        case STATUS_INVALID_ARGUMENTS:
            printf(COLOR_RED"%s Failed: Invalid argument.\n"COLOR_RESET,command);
            break;
        case STATUS_EMPTY_BOARD:
            printf(COLOR_MG"%s Failed: Board Empty.\n"COLOR_RESET,command);
            break;
        case STATUS_TASK_NOT_FOUND:
            printf(COLOR_RED"%s Failed: Not found Task.\n"COLOR_RESET,command);
            break;
        case STATUS_EMPTY_TASK:
            printf(COLOR_MG"%s Failed: Empty Task.\n"COLOR_RESET,command);
            break;
        case STATUS_BOARD_NOT_EMPTY:
            printf(COLOR_MG"%s Failed: Board not empty.\n"COLOR_RESET,command);
            break;
        case STATUS_SAVE_BOARD_FAIL:
            printf(COLOR_RED"%s Failed: save board Failed \n"COLOR_RESET,command);
            break;
        case STATUS_ADD_BOARD_FAIL:
            printf(COLOR_RED"%s Failed: Add Board Failed \n"COLOR_RESET,command);
            break;
        case STATUS_SAVE_TASK_FAIL:
            printf(COLOR_RED"%s Failed: Save Task Failed\n"COLOR_RESET,command);
            break;
        case STATUS_ADD_TASK_FAIL:
            printf(COLOR_RED"%s Failed: Add Task Failed\n"COLOR_RESET,command);
            break;
        default:
            printf(COLOR_RED"%s Failed (Status: %d)\n"COLOR_RESET,
                   command, statusCode);
            break;
    }
    // if (strcmp(status, "0") == 0) {
    //     printf("%s Done\n", command);
    // } else {
    //     printf("%s Failed (Status: %s)\n", command, status);
    // }
    return 0;
}

int readInput(char *buffer, size_t size){
    if (buffer == NULL || size < 2) return -1;
    if (fgets(buffer, size, stdin) == NULL) return -1;

    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n'){
        buffer[length - 1] = '\0';
    }
    else{
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF)
            ;//sdddddddddddddddddddddddddddddddddddddddddddddddd
        printf("Input is too long.\n");
        return -1;
    }

    int hasText = 0;
    for (size_t i = 0; buffer[i] != '\0'; i++){
        if (!isspace((unsigned char)buffer[i]))
        {
            hasText = 1;
            break;
        }
    }
    if (!hasText){
        printf("Input cannot be empty.\n");
        return -1;
    }
    if (strchr(buffer, ',') != NULL || strchr(buffer, '\r') != NULL){
        printf("Input cannot contain ',' or carriage return.\n");
        return -1;
    }
    return 0;
}

void addBoard(int sock){
    char title[BOARD_TITLE_SIZE];
    char request[REQUEST_SIZE];

    printf("Enter Board Title: ");
    if(readInput(title, sizeof(title)) != 0) return;

    int n = snprintf(request, sizeof(request),"%s,%s\n",COMMAND_ADD_BOARD, title);
    if (n < 0 || (size_t)n >= sizeof(request)){
        printf(COLOR_RED "Request is too long.\n" COLOR_RESET);
        return;
    }
    if (sendRequest(sock, request) != 0){
        printf(COLOR_RED "Failed to send request.\n" COLOR_RESET);
        return;
    }
    receiveResponse(sock);
}

void addTask(int sock){
    char boardId[BOARD_TITLE_SIZE];
    char task[TASK_TITLE_SIZE];
    char request[REQUEST_SIZE];

    printf("Enter Board ID: ");
    if (fgets(boardId, sizeof(boardId), stdin) == NULL)
        return;
    boardId[strcspn(boardId, "\n")] = '\0';
    
    printf("Enter Task(500) : ");
    if(fgets(task, sizeof(task), stdin) == NULL) return;

    task[strcspn(task, "\n")] = '\0';
    snprintf(request, sizeof(request),"Add_Task,%s,%s\n",boardId, task);

    printf("Sending: %s", request);
    if(sendRequest(sock, request) == 0){
        receiveResponse(sock);
    } 
}

static int parseBoardResponse(char *response){
    char *save = NULL;
    char *command = strtok_r(response, ",\n\r", &save);
    char *status  = strtok_r(NULL, ",\n\r", &save);
    char *countStr = strtok_r(NULL, ",\n\r", &save);

    if(!command || !status || !countStr){
        printf("Invalid board response.\n");
        return -1;
    }
    if(strcmp(command, COMMAND_GET_BOARDS) != 0){
        printf("Unexpected command: %s\n", command);
        return -1;
    }

    if(strcmp(status, "0") != 0){
        printf("Get Boards Failed (Status: %s)\n", status);
        return -1;
    }
    char *end;
    long count = strtol(countStr, &end, 10);
    if (*end != '\0' || count < 0 || count > MAX_BOARD){
        printf("Invalid board count.\n");
        return -1;
    }
    printf("\n%-10s | %s\n", "Board ID", "Board Title");
    printf("-------------------------------\n");
    for(long i = 0; i < count; i++){
        char *boardId = strtok_r(NULL, ",\n\r", &save);
        char *boardTitle = strtok_r(NULL, ",\n\r", &save);
        if(!boardId || !boardTitle){
            printf("Incomplete board data.\n");
            return -1;
        }
        printf("%-10s | %s\n", boardId, boardTitle);
    }
    return 0;
}
static int receiveLine(int sock, char *buffer, size_t capacity){
    if (buffer == NULL || capacity < 2) return -1;
    size_t index = 0;
    while(index < capacity - 1){
        char ch;
        ssize_t n = recv(sock, &ch, 1, 0);
        if(n <= 0) return -1;
        buffer[index++] = ch;
        if(ch == '\n'){
            buffer[index] = '\0';
            return 0;
        }
    }
    buffer[index] = '\0';
    return -1;
}
static int receiveBoardResponse(int sock){
    char response[64 + MAX_BOARD * (BOARDID_SIZE + BOARD_TITLE_SIZE + 2)];
    char line[BOARDID_SIZE + BOARD_TITLE_SIZE + 2];

    if (receiveLine(sock, response, sizeof(response)) != 0) return -1;

    char header[128];
    size_t headerLen = strlen(response);

    if (headerLen >= sizeof(header)) return -1;
    memcpy(header, response, headerLen + 1);

    char *save = NULL;
    char *command = strtok_r(header, ",\n\r", &save);
    char *status = strtok_r(NULL, ",\n\r", &save);
    char *countStr = strtok_r(NULL, ",\n\r", &save);

    if (!command || !status || strcmp(command, COMMAND_GET_BOARDS) != 0) return -1;
    if (strcmp(status, "0") != 0) return parseBoardResponse(response);
    if (!countStr) return -1;

    char *end = NULL;
    long count = strtol(countStr, &end, 10);
    if (*end != '\0' || count < 0 || count > MAX_BOARD) return -1;

    size_t offset = strlen(response);

    for (long i = 0; i < count; i++){
        if (receiveLine(sock, line, sizeof(line)) != 0) return -1;
        size_t length = strlen(line);
        if (length >= sizeof(response) - offset) return -1;

        memcpy(response + offset, line, length + 1);
        offset += length;
    }
    return parseBoardResponse(response);
}
void readBoard(int sock){
    char request[REQUEST_SIZE];
    snprintf(request, sizeof(request),"%s\n", COMMAND_GET_BOARDS);
    if(sendRequest(sock, request) != 0){
        printf("Failed to send request.\n");
        return;
    }

    if (receiveBoardResponse(sock) != 0){
        printf("Failed to receive board list.\n");
    }
}

static int parseTaskResponse(char *response){
    if (response == NULL) return -1;

    char *save = NULL;
    char *command = strtok_r(response, ",\n\r", &save);
    char *status = strtok_r(NULL, ",\n\r", &save);

    if (!command || !status) return -1;
    if (strcmp(command, COMMAND_GET_TASK) != 0) return -1;
    if (strcmp(status, "0") != 0){
        printf("Get Task Failed (Status: %s)\n", status);
        return -1;
    }

    char *countStr = strtok_r(NULL, ",\n\r", &save);
    if (!countStr) return -1;

    char *end = NULL;
    long count = strtol(countStr, &end, 10);

    if (*end != '\0' || count < 0 || count > MAX_TASK) return -1;
    printf("\n%-10s | %-30s | %s\n","Task ID", "Task Title", "Status");
    printf("----------------------------------------------------------\n");
    for (long i = 0; i < count; i++){
        char *taskId = strtok_r(NULL, ",\n\r", &save);
        char *taskTitle = strtok_r(NULL, ",\n\r", &save);
        char *isFinish = strtok_r(NULL, ",\n\r", &save);

        if (!taskId || !taskTitle || !isFinish){
            printf("Incomplete task data.\n");
            return -1;
        }
        if(strcmp(isFinish, "0") != 0 && strcmp(isFinish, "1") != 0){
            printf("Invalid task status.\n");
            return -1;
        }
        printf("%-10s | %-30s | %s\n",taskId,taskTitle,strcmp(isFinish, "1") == 0 ? "Done" : "Pending");
    }
    return 0;
}
int receiveTaskResponse(int sock){
    char response[64 + MAX_TASK * (TASKID_SIZE + TASK_TITLE_SIZE + 8)];
    char line[TASKID_SIZE + TASK_TITLE_SIZE + 8];

    if (receiveLine(sock, response, sizeof(response)) != 0) return -1;

    char header[128];
    size_t headerLen = strlen(response);
    if (headerLen >= sizeof(header)) return -1;

    memcpy(header, response, headerLen + 1);
    char *save = NULL;
    char *command = strtok_r(header, ",\n\r", &save);
    char *status = strtok_r(NULL, ",\n\r", &save);

    if (!command || !status) return -1;
    if (strcmp(command, COMMAND_GET_TASK) != 0)  return -1;
    if (strcmp(status, "0") != 0) return parseTaskResponse(response);

    char *countStr = strtok_r(NULL, ",\n\r", &save);
    if (!countStr) return -1;

    char *end = NULL;
    long count = strtol(countStr, &end, 10);
    if (*end != '\0' || count < 0 || count > MAX_TASK) return -1;

    size_t offset = strlen(response);
    for (long i = 0; i < count; i++){
        if (receiveLine(sock, line, sizeof(line)) != 0) return -1;

        size_t length = strlen(line);
        if (length >= sizeof(response) - offset) return -1;
        memcpy(response + offset, line, length + 1);
        offset += length;
    }
    return parseTaskResponse(response);
}
void readTask(int sock){
    char boardId[BOARDID_SIZE];
    char request[REQUEST_SIZE];

    printf("Enter Board ID: ");
    if (fgets(boardId, sizeof(boardId), stdin) == NULL) return;
    boardId[strcspn(boardId, "\n")] = '\0';
    if (boardId[0] == '\0'){
        printf("Board ID cannot be empty.\n");
        return;
    }

    snprintf(request, sizeof(request),"%s,%s\n", COMMAND_GET_TASK, boardId);
    if (sendRequest(sock, request) != 0){
        printf("Failed to send request.\n");
        return;
    }
    if (receiveTaskResponse(sock) != 0){
        printf("Failed to receive task list.\n");
    }
}

void updateBoard(int sock){
    char boardId[BOARDID_SIZE];
    char newTitle[BOARD_TITLE_SIZE];
    char request[BOARDID_SIZE + BOARD_TITLE_SIZE + 64];

    printf("Enter Board ID: ");
    if (readInput(boardId, sizeof(boardId)) != 0) return;
    printf("Enter New Board Title: ");

    if (readInput(newTitle, sizeof(newTitle)) != 0) return;
    int n = snprintf(request, sizeof(request),"%s,%s,%s\n", COMMAND_UPDATE_BOARD,boardId,newTitle);
    if(n < 0 || (size_t)n >= sizeof(request)){
        printf("Request is too long.\n");
        return;
    }
    if (sendRequest(sock, request) != 0){
        printf("Failed to send request.\n");
        return;
    }
    receiveResponse(sock);
}

void updateTask(int sock){
    char boardId[BOARDID_SIZE];
    char taskId[TASKID_SIZE];
    char newTitle[TASK_TITLE_SIZE];
    char request[BOARDID_SIZE + TASKID_SIZE + TASK_TITLE_SIZE + 64];

    printf("Enter Board ID: ");
    if (readInput(boardId, sizeof(boardId)) != 0) return;

    printf("Enter Task ID: ");
    if (readInput(taskId, sizeof(taskId)) != 0) return;

    printf("Enter New Task Title: ");
    if (readInput(newTitle, sizeof(newTitle)) != 0) return;

    int n = snprintf(request, sizeof(request),"%s,%s,%s,%s\n",COMMAND_UPDATE_TASK,boardId,taskId,newTitle);
    if (n < 0 || (size_t)n >= sizeof(request)){
        printf("Request is too long.\n");
        return;
    }
    if (sendRequest(sock, request) != 0){
        printf("Failed to send request.\n");
        return;
    }
    if (receiveResponse(sock) != 0){
        printf("Failed to receive server response.\n");
    }
}
void updateTaskStatus(int sock){
    char boardId[BOARDID_SIZE];
    char taskId[TASKID_SIZE];
    char status[10];
    char request[REQUEST_SIZE];

    printf("Enter Board ID: ");
    if (fgets(boardId, sizeof(boardId), stdin) == NULL) return;
    boardId[strcspn(boardId, "\n")] = '\0';

    printf("Enter Task ID: ");
    if (fgets(taskId, sizeof(taskId), stdin) == NULL) return;
    taskId[strcspn(taskId, "\n")] = '\0';

    printf("Enter Status (0 = Pending, 1 = Done): ");
    if (fgets(status, sizeof(status), stdin) == NULL) return;
    status[strcspn(status, "\n")] = '\0';

    if(strcmp(status, "0") != 0 &&strcmp(status, "1") != 0) {
        printf("Invalid status.\n");
        return;
    }
    snprintf(request, sizeof(request),"Update_Task_Status,%s,%s,%s\n",boardId, taskId, status);
    printf("Sending: %s", request);
    if(sendRequest(sock, request) == 0) {
        receiveResponse(sock);
    }
}
void deleteBoard(int sock){
    char boardId[BOARDID_SIZE];
    char request[BOARDID_SIZE + 64];

    printf("Enter Board ID: ");
    if (readInput(boardId, sizeof(boardId)) != 0) return;
    int n = snprintf(request, sizeof(request),"%s,%s\n",COMMAND_DELETE_BOARD, boardId);

    if (n < 0 || (size_t)n >= sizeof(request)){
        printf("Request is too long.\n");
        return;
    }
    if (sendRequest(sock, request) != 0){
        printf("Failed to send request.\n");
        return;
    }
    if (receiveResponse(sock) != 0){
        printf("Failed to receive server response.\n");
    }
}
void deleteTask(int sock){
    char boardId[BOARDID_SIZE];
    char taskId[TASKID_SIZE];
    char request[BOARDID_SIZE + TASKID_SIZE + 64];

    printf(COLOR_BLUE"Enter Board ID: "COLOR_RESET);
    if (readInput(boardId, sizeof(boardId)) != 0) return;

    printf(COLOR_BLUE"Enter Task ID: "COLOR_RESET);
    if (readInput(taskId, sizeof(taskId)) != 0) return;

    int n = snprintf(request, sizeof(request),"%s,%s,%s\n",COMMAND_DELETE_TASK,boardId,taskId);
    if (n < 0 || (size_t)n >= sizeof(request)){
        printf(COLOR_RED"Request is too long.\n"COLOR_RESET);
        return;
    }
    if (sendRequest(sock, request) != 0){
        printf(COLOR_RED"Failed to send request.\n"COLOR_RESET);
        return;
    }
    if (receiveResponse(sock) != 0){
        printf(COLOR_RED"Failed to receive server response.\n"COLOR_RESET);
    }
}

int main(){
    int sock = connectToServer();
    if(sock < 0){
        printf(COLOR_RED"Cannot connect to server.\n"COLOR_RESET);
        return 1;
    }

    printf(COLOR_GREEN"Connected to ProjectOS Server!\n"COLOR_RESET);
    char input[32];
    while(1){
        printf("\n");
        printf(COLOR_CYAN COLOR_BOLD "============= PROJECT OS MENU =============\n" COLOR_RESET);

        printf(COLOR_CYAN "|%-15s|%-20s|\n" COLOR_RESET, "No.", "Menu");
        printf(COLOR_CYAN "---------------------------------------\n" COLOR_RESET);

        printf(COLOR_CYAN "|%-15s|" COLOR_RESET COLOR_WH "%-20s" COLOR_CYAN "|\n" COLOR_RESET, "1", "Add Board");
        printf(COLOR_CYAN "|%-15s|" COLOR_RESET COLOR_WH "%-20s" COLOR_CYAN "|\n" COLOR_RESET, "2", "Add Task");
        printf(COLOR_CYAN "|%-15s|" COLOR_RESET COLOR_WH "%-20s" COLOR_CYAN "|\n" COLOR_RESET, "3", "Read Boards");
        printf(COLOR_CYAN "|%-15s|" COLOR_RESET COLOR_WH "%-20s" COLOR_CYAN "|\n" COLOR_RESET, "4", "Read Tasks");
        printf(COLOR_CYAN "|%-15s|" COLOR_RESET COLOR_WH "%-20s" COLOR_CYAN "|\n" COLOR_RESET, "5", "Update Board");
        printf(COLOR_CYAN "|%-15s|" COLOR_RESET COLOR_WH "%-20s" COLOR_CYAN "|\n" COLOR_RESET, "6", "Update Task");
        printf(COLOR_CYAN "|%-15s|" COLOR_RESET COLOR_WH "%-20s" COLOR_CYAN "|\n" COLOR_RESET, "7", "Update Task Status");

        printf(COLOR_CYAN "|%-15s|" COLOR_RESET COLOR_RED "%-20s" COLOR_CYAN "|\n" COLOR_RESET, "8", "Delete Board");
        printf(COLOR_CYAN "|%-15s|" COLOR_RESET COLOR_RED "%-20s" COLOR_CYAN "|\n" COLOR_RESET, "9", "Delete Task");
        printf(COLOR_CYAN "|%-15s|" COLOR_RESET COLOR_RED "%-20s" COLOR_CYAN "|\n" COLOR_RESET, "0", "Exit");

        printf(COLOR_CYAN "---------------------------------------\n" COLOR_RESET);

        printf("\n" COLOR_GREEN COLOR_BOLD "Select option: " COLOR_RESET);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        char *end;
        long choice = strtol(input, &end, 10);
        if (end == input || (*end != '\n' && *end != '\0')) {
            printf(COLOR_RED"Invalid option.\n"COLOR_RESET);
            continue;
        }
        switch (choice){
            case 1:
                addBoard(sock);
                break;
            case 2:
                addTask(sock);
                break;
            case 3:
                readBoard(sock);
                break;
            case 4:
                readTask(sock);
                break;
            case 5:
                updateBoard(sock);
                break;
            case 6:
                updateTask(sock);
                break;
            case 7:
                updateTaskStatus(sock);
                break;
            case 8:
                deleteBoard(sock);
                break;
            case 9:
                deleteTask(sock);
                break;
            case 0:
                printf(COLOR_YELLOW"Goodbye Boooooy!\n"COLOR_RESET);
                close(sock);
                return 0;
            default:
                printf(COLOR_RED"Invalid option.\n"COLOR_RESET);
                break;
        }
        if (!serverConnected){
            printf(COLOR_RED"\n[ERROR] Connection to server lost.\n"COLOR_RESET);
            printf(COLOR_YELLOW"Closing client...\n"COLOR_RESET);
            break;
        }
    }
    close(sock);
    return 0;
}