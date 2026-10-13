#ifndef CLIENT_H
#define CLIENT_H

#define BOARDID_SIZE 16
#define BOARD_TITLE_SIZE 500
#define TASKID_SIZE 16
#define TASK_TITLE_SIZE 500
#define MAX_TASK 100
#define MAX_BOARD 100
#define REQUEST_SIZE 500
#define RESPONSE_SIZE 1024

int connectToServer(void);
int sendRequest(int socket, const char *request);
int receiveResponse(int socket);

void addBoard(int socket);
void addTask(int socket); 
void readBoard(int socket);
void readTask(int socket);
void updateBoard(int socket);
void updateTask(int socket);
void updateTaskStatus(int sock);
void deleteBoard(int socket);
void deleteTask(int socket);

#endif