#include "server.h"
#include "models.h"
#include <signal.h>

#include <semaphore.h>
#include <fcntl.h>    /* For O_CREAT, O_RDWR constants */
#include <sys/stat.h> /* For mode constants like 0644 */

SOCKET socket_listen = -1;
Project *project = NULL;
sem_t *sem = NULL;

void handleAddBoard(CommandContext *ctx){
  char *boardTitle = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);

  char response[MESSAGE_SIZE];

  if (isEmptyOrSpace(boardTitle))
  {
    snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_BOARD, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
    send(ctx->clientSocket, response, strlen(response), 0);
  }
  else
  {
    // 1. Wait for the lock (Block if another process is buying)
    sem_wait(sem);

    // 2. CRITICAL SECTION: Only one process can be here at a time
    int result = addBoard(ctx->project, boardTitle);
    saveBoard(ctx->project, BOARD_FILENAME);
    // 3. Release the lock
    sem_post(sem);
    if (result == 0)
    {
      snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_BOARD, COMMAND_SEPARATOR, STATUS_OK);
      send(ctx->clientSocket, response, strlen(response), 0);
    }
    else
    {
      snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_BOARD, COMMAND_SEPARATOR, STATUS_FAIL);
      send(ctx->clientSocket, response, strlen(response), 0);
    }
  }
}
void handleAddTask(CommandContext *ctx)
{
    char *boardId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
    char *taskTitle = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);

    char response[MESSAGE_SIZE];

    if (boardId == NULL || taskTitle == NULL ||
        isEmptyOrSpace(boardId) || isEmptyOrSpace(taskTitle))
    {
        snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_TASK, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    Board *board = hasBoard(ctx->project, boardId);

    if (board == NULL)
    {
        snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_TASK, COMMAND_SEPARATOR, STATUS_BOARD_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // 1. Wait for the lock
    sem_wait(sem);

    // 2. CRITICAL SECTION
    int result = addTask(board, taskTitle);
    saveAllTasks(ctx->project, TASK_FILENAME);

    // 3. Release the lock
    sem_post(sem);

    if (result == 0)
    {
        snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_TASK, COMMAND_SEPARATOR, STATUS_OK);
    }
    else
    {
        snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_ADD_TASK, COMMAND_SEPARATOR, STATUS_FAIL);
    }
    send(ctx->clientSocket, response, strlen(response), 0);
}
void handleGetBoards(CommandContext *ctx){
  char response[64+MAX_BOARD*(BOARDID_SIZE+BOARD_TITLE_SIZE+2)];
  size_t offset = 0;
  sem_wait(sem);

  int count = ctx->project->totalBoards;
    int n = snprintf(response,sizeof(response),"%s,%d,%d\n",COMMAND_GET_BOARDS,STATUS_OK,count);
    if(n < 0 || (size_t)n >= sizeof(response)){
        sem_post(sem);
        return;
    }
    offset = (size_t)n;
    for (int i = 0; i < count; i++){
        Board *board = &ctx->project->boards[i];
        n = snprintf(response + offset,sizeof(response) - offset,"%s,%s\n",board->boardId,board->boardTitle);
        if(n < 0 || (size_t)n >= sizeof(response) - offset){
            sem_post(sem);
            return;
        }
        offset += (size_t)n;
    }
    sem_post(sem);
    size_t sent = 0;
    while (sent < offset){
        ssize_t nsend = send(ctx->clientSocket,response + sent,offset - sent,0);
        if (nsend <= 0) return;
        sent += (size_t)nsend;
    }
}
void handleGetTask(CommandContext *ctx){
  char *boardId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
    if(boardId ==NULL || isEmptyOrSpace(boardId)){
      char response[RESPONSE_SIZE];
      snprintf(response, sizeof(response), "%s,%d\n",COMMAND_GET_TASK, STATUS_INVALID_ARGUMENTS);
      send(ctx->clientSocket, response, strlen(response), 0);
      return;
    }

    sem_wait(sem);
    Board *board = hasBoard(ctx->project, boardId);
    if(board== NULL) {
      sem_post(sem);
      char response[RESPONSE_SIZE];
      snprintf(response, sizeof(response), "%s,%d\n",COMMAND_GET_TASK, STATUS_BOARD_NOT_FOUND);
      send(ctx->clientSocket, response, strlen(response), 0);
      return;
    }
    char response[64 + MAX_TASK * (TASKID_SIZE + TASK_TITLE_SIZE + 8)];
    size_t used = 0;
    used += snprintf(response, sizeof(response),"Get_Task,0,%d\n", board->totalTasks);

    for (int i = 0; i < board->totalTasks; i++) {
        Task *task = &board->tasks[i];
        used += snprintf(response + used, sizeof(response) - used,"%s,%s,%d\n",
                        task->taskId,task->taskTitle,task->isFinish);
    }
    sem_post(sem); // ส่งข้อมูลกลับ Client
  
    size_t sent = 0;
    while (sent < used) {
        ssize_t n = send(ctx->clientSocket,response + sent, used - sent, 0);
        if(n <= 0){
            perror("send");
            return;
        }
        sent += (size_t)n;
    }
}
void handleUpdateTaskStatus(CommandContext *ctx){
    char *boardId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
    char *taskId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
    char *status = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);

    if(boardId == NULL || taskId == NULL || status == NULL ||isEmptyOrSpace(boardId) || isEmptyOrSpace(taskId) ||
        (strcmp(status, "0") != 0 && strcmp(status, "1") != 0)) {
      char response[RESPONSE_SIZE];
      snprintf(response,sizeof(response),"%s,%d\n",COMMAND_UPDATE_TASK_STATUS,STATUS_INVALID_ARGUMENTS);
      send(ctx->clientSocket, response, strlen(response), 0);
      return;
    }

    sem_wait(sem);
    Board *board = hasBoard(ctx->project, boardId);
    if(board == NULL){
      sem_post(sem);
      char response[RESPONSE_SIZE];
      snprintf(response,sizeof(response),"%s,%d\n",COMMAND_UPDATE_TASK_STATUS,STATUS_BOARD_NOT_FOUND);
      send(ctx->clientSocket, response, strlen(response), 0);
      return;
    }

    Task *task = hasTask(ctx->project, boardId, taskId);
    if(task == NULL){
      sem_post(sem);
      char response[RESPONSE_SIZE];
      snprintf(response,sizeof(response),"%s,%d\n",COMMAND_UPDATE_TASK_STATUS,STATUS_TASK_NOT_FOUND);
      send(ctx->clientSocket, response, strlen(response), 0);
      return;
    }

    int oldStatus = task->isFinish;
    task->isFinish = status[0] - '0';

    int result = saveAllTasks(ctx->project, TASK_FILENAME);
    if (result != 0) {
      task->isFinish = oldStatus;
      sem_post(sem);
      char response[RESPONSE_SIZE];
      snprintf(response,sizeof(response),"%s,%d\n",COMMAND_UPDATE_TASK_STATUS,STATUS_SAVE_TASK_FAIL);
      send(ctx->clientSocket, response, strlen(response), 0);
      return;
    }

    sem_post(sem);
    char response[RESPONSE_SIZE];
    snprintf(response,sizeof(response),"%s,%d\n",COMMAND_UPDATE_TASK_STATUS,STATUS_OK);
    send(ctx->clientSocket, response, strlen(response), 0);
}
void handleUpdateBoard(CommandContext *ctx){
    char *boardId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
    char *newTitle = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
    if(boardId == NULL || newTitle == NULL ||isEmptyOrSpace(boardId) || isEmptyOrSpace(newTitle) ||
      strlen(newTitle) >= BOARD_TITLE_SIZE){
      char response[100];
      snprintf(response, sizeof(response),"%s,%d\n", COMMAND_UPDATE_BOARD, STATUS_INVALID_ARGUMENTS);
      send(ctx->clientSocket, response, strlen(response), 0);
      return;
    }
    sem_wait(sem);
    Board *board = hasBoard(ctx->project, boardId);
    if(board == NULL){
      sem_post(sem);
      char response[100];
      snprintf(response, sizeof(response),"%s,%d\n", COMMAND_UPDATE_BOARD, STATUS_BOARD_NOT_FOUND);
      send(ctx->clientSocket, response, strlen(response), 0);
      return;
    }

    char oldTitle[BOARD_TITLE_SIZE];
    strcpy(oldTitle, board->boardTitle);
    strcpy(board->boardTitle, newTitle);

    int result = updateBoard(ctx->project, boardId, newTitle);
    if(result != 0) strcpy(board->boardTitle, oldTitle);

    sem_post(sem);
    if(result != 0){
      char response[100];
      snprintf(response, sizeof(response),"%s,%d\n", COMMAND_UPDATE_BOARD, STATUS_SAVE_BOARD_FAIL);
      send(ctx->clientSocket, response, strlen(response), 0);
      return;
    }
    char response[100];
    snprintf(response, sizeof(response),"%s,%d\n", COMMAND_UPDATE_BOARD, STATUS_OK);
    send(ctx->clientSocket, response, strlen(response), 0);
}
void handleUpdateTask(CommandContext *ctx){
    char *boardId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
    char *taskId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
    char *newTitle = strtok_r(NULL, "\n\r", &ctx->rawInput);
    if(!boardId || !taskId || !newTitle ||isEmptyOrSpace(newTitle) ||strlen(newTitle) >= TASK_TITLE_SIZE){
      // send(ctx->clientSocket,sizeof(&newTitle),COMMAND_UPDATE_TASK,STATUS_INVALID_ARGUMENTS);
      char response[RESPONSE_SIZE];
      snprintf(response, sizeof(response),"%s,%d\n", COMMAND_UPDATE_BOARD, STATUS_INVALID_ARGUMENTS);
      send(ctx->clientSocket, response, strlen(response), 0);
      return;
    }
    sem_wait(sem);
    Task *task = hasTask(ctx->project, boardId, taskId);
    if(task == NULL){
      sem_post(sem);
      char response[RESPONSE_SIZE];
      snprintf(response,sizeof(response),"%s,%d\n",COMMAND_UPDATE_TASK_STATUS,STATUS_TASK_NOT_FOUND);
      send(ctx->clientSocket, response, strlen(response), 0);
      return;
    }
    char oldTitle[TASK_TITLE_SIZE];
    strcpy(oldTitle, task->taskTitle);
    strcpy(task->taskTitle, newTitle);

    int result = updateTask(ctx->project, boardId, taskId, newTitle);
    if (result != 0) strcpy(task->taskTitle, oldTitle);
    sem_post(sem);
    // send(ctx->clientSocket,COMMAND_UPDATE_TASK,result == 0 ? STATUS_OK : STATUS_SAVE_TASK_FAIL);
    char response[RESPONSE_SIZE];
    snprintf(response,sizeof(response),"%s,%d\n",COMMAND_UPDATE_TASK_STATUS,result == 0 ? STATUS_OK : STATUS_SAVE_TASK_FAIL);
    send(ctx->clientSocket, response, strlen(response), 0);
}

void handleDeleteTask(CommandContext *ctx){
  char *boardId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
  char *taskId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
  char response[MESSAGE_SIZE];
  int status;
  if (isEmptyOrSpace(boardId) || isEmptyOrSpace(taskId)){
      status = STATUS_INVALID_ARGUMENTS;
  }else{
      sem_wait(sem);
      Board *board = hasBoard(ctx->project, boardId);
      if (board == NULL){
          status = STATUS_BOARD_NOT_FOUND;
      }else{
          Task *task = hasTask(ctx->project, boardId, taskId);
          if (task == NULL){
              status = STATUS_TASK_NOT_FOUND;
          }else{
              int result = deleteTask(ctx->project, boardId, taskId);
              status = (result == 0)? STATUS_OK: STATUS_SAVE_TASK_FAIL;
          }
      }
      sem_post(sem);
  }
  snprintf(response, sizeof(response), "%s%s%d\n",COMMAND_DELETE_TASK,COMMAND_SEPARATOR,status);
  send(ctx->clientSocket, response, strlen(response), 0);
}
void handleDeleteBoard(CommandContext *ctx){
  char *boardId = strtok_r(NULL, COMMAND_SEPARATOR, &ctx->rawInput);
  char response[MESSAGE_SIZE];
  int status;
  if (isEmptyOrSpace(boardId)){
      status = STATUS_INVALID_ARGUMENTS;
  }else{
      sem_wait(sem);
      Board *board = hasBoard(ctx->project, boardId);
      if (board == NULL){
        status = STATUS_BOARD_NOT_FOUND;
      }
      else if (board->totalTasks > 0){
          status = STATUS_BOARD_NOT_EMPTY;
      }else{
          int result = deleteBoard(ctx->project, boardId);
          status = (result == 0)? STATUS_OK: STATUS_SAVE_BOARD_FAIL;
      }
      sem_post(sem);
  }
  snprintf(response, sizeof(response),"%s%s%d\n",COMMAND_DELETE_BOARD,COMMAND_SEPARATOR,status);
  send(ctx->clientSocket, response, strlen(response), 0);
}
int handleClient(SOCKET socket_client, char *read){
  // printf("line: %s\n", read);
  char response[MESSAGE_SIZE];
  char *saveptr;
  char *commandName = strtok_r(read, ",\n\r", &saveptr);
  printf("DEBUG commandName: [%s]\n",
       commandName != NULL ? commandName : "NULL");
       
  if (commandName == NULL)
    return -1;

  CommandContext ctx = {
      .project = project,
      .clientSocket = socket_client,
      .rawInput = saveptr,
  };

  CommandEntry commandTable[] = {
      {COMMAND_ADD_BOARD, handleAddBoard},
      {COMMAND_ADD_TASK, handleAddTask},
      {COMMAND_GET_BOARDS,handleGetBoards},
      {COMMAND_GET_TASK,handleGetTask},
      {COMMAND_UPDATE_BOARD,handleUpdateBoard},
      {COMMAND_UPDATE_TASK,handleUpdateTask},
      {COMMAND_UPDATE_TASK_STATUS,handleUpdateTaskStatus},
      {COMMAND_DELETE_TASK, handleDeleteTask},
      {COMMAND_DELETE_BOARD, handleDeleteBoard},
      {NULL, NULL} // Sentinel to mark the end
  };

  for (int i = 0; commandTable[i].commandName != NULL; i++)
  {
    if (strcmp(commandName, commandTable[i].commandName) == 0)
    {
      // Found the command! Execute its function.
      printf("Found command: %s\n", commandName);
      commandTable[i].handler(&ctx);
      return 0;
    }
  }
  snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_UNKNOWN, COMMAND_SEPARATOR, STATUS_UNKNOWN);
  send(socket_client, response, strlen(response), 0);
  return -1;
}
void handle_shutdown(int sig)
{
  if (sig == SIGINT)
  {
    printf("\nCaught Ctrl+C! Cleaning up...\n");
  }
  else if (sig == SIGTERM)
  {
    printf("\nTermination request received...\n");
  }
  // To prevent an infinite loop of signals
  signal(SIGINT, SIG_IGN);

  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen);
  socket_listen = -1;

  printf("Cleaning up semaphore...\n");
  sem_close(sem);
  sem_unlink("/store_lock");

  // 2. Kill all child processes in the group
  // Passing 0 as the PID sends the signal to everyone in the process group
  kill(0, SIGTERM);

  exit(0);
}

int setup(char *port)
{
  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // init semaphore
  sem = sem_open("/store_lock", O_CREAT, 0644, 1);

  // Init project using shared memory (mmap)
  project = mmap(NULL, sizeof(Project), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  loadProject(project, BOARD_FILENAME, TASK_FILENAME);

  struct addrinfo *bind_address;
  getaddrinfo(0, port, &hints, &bind_address);

  printf("Creating socket...\n");

  socket_listen = socket(bind_address->ai_family, 
                        bind_address->ai_socktype,
                        bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen))
  {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  // Allow socket address reuse
  int yes = 1;
  if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
  {
    fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  printf("Binding socket to local address...\n");
  printf("Binding to port: %s\n", SERVER_PORT);
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
  {
    fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }
  freeaddrinfo(bind_address);

  printf("Listening...\n");
  if (listen(socket_listen, 10) < 0)
  {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  printf("Waiting for connections...\n");
  while (1)
  {
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    SOCKET socket_client =
        accept(socket_listen, (struct sockaddr *)&client_address, &client_len);
    if (!ISVALIDSOCKET(socket_client))
    {
      // If we are shutting down, don't print an error
      if (socket_listen == -1)
      {
        return 0;
      }
      fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
    }

    char address_buffer[100];
    getnameinfo((struct sockaddr *)&client_address, client_len, address_buffer,
                sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    printf("New connection from %s\n", address_buffer);

    int pid = fork();
    if (pid == 0)
    { // child process
      CLOSESOCKET(socket_listen);

      while (1)
      {
        char read[1024];
        int bytes_received = recv(socket_client, read, sizeof(read) - 1, 0);
        if (bytes_received < 1)
        {
          printf("Client disconnected.\n");

          CLOSESOCKET(socket_client);
          exit(0);
        }
        read[bytes_received] = '\0';
        
        int j;
        for (j = 0; j < bytes_received; ++j)
          read[j] = read[j];

        printf("%s\n", read);
        replace_char(read, '\n', '\0');
        handleClient(socket_client, read);
      }
    }
    CLOSESOCKET(socket_client);
  } // while(1)

  printf("Finished.\n");
  return 0;
}

int main()
{

  signal(SIGINT, handle_shutdown);  // Catches CTRL+C
  signal(SIGTERM, handle_shutdown); // Catches kill command
  setup(SERVER_PORT);

  return 0;
}