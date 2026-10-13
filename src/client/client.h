#ifndef CLIENT_H
#define CLIENT_H

#define BOARDID_SIZE 16
#define BOARD_TITLE_SIZE 500
#define TASKID_SIZE 16
#define TASK_TITLE_SIZE 500
#define MAX_TASK 100
#define MAX_BOARD 100
#define REQUEST_SIZE 1024
#define RESPONSE_SIZE 1024

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MG    "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WH    "\033[37m"
#define COLOR_BOLD    "\033[1m"

int connectToServer();
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