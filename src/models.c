#include "models.h"

int loadProject(Project *project, char *boardFileName, char *taskFileName){
    if(readBoard(project, boardFileName) == 1){
        return 1;
    }
    if(readAllTasks(project, taskFileName) == 1){
        return 1;
    }
    return 0;
}
int readBoard(Project *project, char *fileName){
    FILE *file_ptr = fopen(fileName, "r");

    if (file_ptr == NULL) {
        // File doesn't exist, let's create it!
        printf("File not found. Creating a new one...\n");
        file_ptr = fopen(fileName, "w+"); // w+ creates it and allows reading
    }

    if (file_ptr == NULL)
        return 1;

    char buffer[BOARDID_SIZE + BOARD_TITLE_SIZE + 2];
    int i = 0;
    int fd = fileno(file_ptr);
    flock(fd, LOCK_EX);
    while (fgets(buffer, sizeof(buffer), file_ptr) != NULL && i < MAX_BOARD)
    {
        // 1. Remove newline
        buffer[strcspn(buffer, "\n")] = 0;

        // 2. Correct Tokenizing
        char *boardId = strtok(buffer, ",");
        char *boardTitle = strtok(NULL, ",");

        // 3. Check if tokens exist before using them
        if (boardId != NULL && boardTitle != NULL)
        {
            strncpy(project->boards[i].boardId,boardId,sizeof(project->boards[i].boardId) - 1);
            project->boards[i].boardId[sizeof(project->boards[i].boardId) - 1] = '\0';
            strncpy(project->boards[i].boardTitle,boardTitle,sizeof(project->boards[i].boardTitle) - 1);
            project->boards[i].boardTitle[sizeof(project->boards[i].boardTitle) - 1] = '\0';
            i++;
        }
    }
    project->totalBoards = i;
    flock(fd, LOCK_UN);
    fclose(file_ptr);
    return 0;
}
int saveBoard(Project *project,  char *fileName){
    FILE *file_ptr;

    file_ptr = fopen(fileName, "w");

    if (file_ptr == NULL)
    {
        printf("Error opening file!\n");
        return 1;
    }
    int fd = fileno(file_ptr);
    flock(fd, LOCK_EX);
    Board *boards = project->boards;
    for (int i = 0; i <project->totalBoards; i++) //วนตามบอร์ดที่มีอยู่ ไม่ต้องวนทั้งหดม
    {
        fprintf(file_ptr, "%s,%s\n", boards[i].boardId, boards[i].boardTitle);
    }
    flock(fd, LOCK_UN);
    fclose(file_ptr);
    return 0;
}
int readAllTasks(Project *project, char *fileName){
    FILE *file_ptr = fopen(fileName, "r");

    if (file_ptr == NULL) {
        printf("File not found. Creating a new one...\n");
        file_ptr = fopen(fileName, "w+");
    }

    if (file_ptr == NULL)
        return 1;

    char buffer[255];
    int fd = fileno(file_ptr);
    flock(fd, LOCK_EX);

    while (fgets(buffer, sizeof(buffer), file_ptr) != NULL)
    {
        // 1. Remove newline
        buffer[strcspn(buffer, "\n")] = 0;

        // 2. Tokenize
        char *boardId = strtok(buffer, ",");
        char *taskId = strtok(NULL, ",");
        char *taskTitle = strtok(NULL, ",");
        char *isFinishStr = strtok(NULL, ",");

        if (boardId == NULL || taskId == NULL || taskTitle == NULL || isFinishStr == NULL)
            continue; // skip malformed line

        // 3. Find the board with this boardId
        Board *targetBoard = NULL;
        for (int b = 0; b < project->totalBoards; b++)
        {
            if (strcmp(project->boards[b].boardId, boardId) == 0)
            {
                targetBoard = &project->boards[b];
                break;
            }
        }

        if (targetBoard == NULL)
            continue; // task belongs to a board we don't have (shouldn't normally happen)

        // 4. Add task to that board (bounds-checked)
        if (targetBoard->totalTasks < MAX_TASK)
        {
            int i = targetBoard->totalTasks;
            strcpy(targetBoard->tasks[i].taskId, taskId);
            strcpy(targetBoard->tasks[i].taskTitle, taskTitle);
            targetBoard->tasks[i].isFinish = atoi(isFinishStr);
            targetBoard->totalTasks++;
        }
    }

    flock(fd, LOCK_UN);
    fclose(file_ptr);
    return 0;
}
int saveAllTasks(Project *project, char *fileName){
    FILE *file_ptr = fopen(fileName, "w");
    if (file_ptr == NULL) {
        printf("Error opening file!\n");
        return 1;
    }
    int fd = fileno(file_ptr);
    flock(fd, LOCK_EX);

    for (int b = 0; b < project->totalBoards; b++) {
        Board *board = &project->boards[b];
        for (int i = 0; i < board->totalTasks; i++) {
            fprintf(file_ptr, "%s,%s,%s,%d\n",
                    board->boardId,
                    board->tasks[i].taskId,
                    board->tasks[i].taskTitle,
                    board->tasks[i].isFinish);
        }
    }

    flock(fd, LOCK_UN);
    fclose(file_ptr);
    return 0;
}
static int getNextBoardId(Project *project){
    int maxId = 0;
    for (int i = 0; i < project->totalBoards; i++){
        int id = atoi(project->boards[i].boardId);
        if (id > maxId) maxId = id;
    }
    return maxId + 1;
}

int addBoard(Project *project, char *boardTitle){
    if(project == NULL || boardTitle ==NULL) return 1;
    if(project->totalBoards >= MAX_BOARD) return 1; 
    if(isEmptyOrSpace(boardTitle)) return 1;
    Board *board = &project->boards[project->totalBoards];
    snprintf(board->boardId,sizeof(board->boardId), "%d" , getNextBoardId(project));
    strncpy(board->boardTitle,boardTitle,sizeof(board->boardTitle)-1);

    board->boardTitle[sizeof(board->boardTitle)-1] = '\0';
    board->totalTasks = 0;
    project->totalBoards++;
    return 0;
}
static int getNextTaskId(Board *board){
    int maxId = 0;
    for (int i = 0; i < board->totalTasks; i++){
        int id = atoi(board->tasks[i].taskId);
        if (id > maxId) maxId = id;
    }
    return maxId + 1;
}
int addTask(Board *board, char *taskTitle){
    if(board == NULL || taskTitle ==NULL) return 1;
    if(board->totalTasks >= MAX_TASK) return 1;
    if(isEmptyOrSpace(taskTitle)) return 1;
    Task *task = &board->tasks[board->totalTasks];
    snprintf(task->taskId, sizeof(task->taskId), "%d", getNextTaskId(board));
    strncpy(task->taskTitle, taskTitle,sizeof(task->taskTitle) - 1);
    task->taskTitle[sizeof(task->taskTitle) - 1] ='\0';
    task->isFinish = 0;
    board->totalTasks++;
    return 0;
}
Board *hasBoard(Project *project,char *boardId){
    Board *board = NULL;
    if(boardId==NULL) return NULL;
    for (int i = 0; i < project->totalBoards; i++)
    {
        if (strcmp(project->boards[i].boardId, boardId) == 0)
        {
            board = &project->boards[i];
            break;
        }
    }
    return board;
}
Task *hasTask(Project *project, char *boardId, char *taskId){
    Board *board = hasBoard(project, boardId);
    if(board==NULL || taskId == NULL) return NULL;
    Task *task = NULL;
    for (int i=0;i < board->totalTasks; i++){
        if (strcmp(board->tasks[i].taskId, taskId) == 0){
            task = & board->tasks[i];
            break;
        }
    }
    return task;
}
int deleteTask(Project *project, char *boardId, char *taskId){
    if (project == NULL || boardId == NULL || taskId == NULL) return 1;

    Board *board = hasBoard(project, boardId);

    if (board == NULL) return 1;
    int index = -1;
    for (int i = 0; i < board->totalTasks; i++){
        if (strcmp(board->tasks[i].taskId, taskId) == 0){
            index = i;
            break;
        }
    }
    if (index == -1) return 1;
    for (int i = index; i < board->totalTasks - 1; i++){
        board->tasks[i] = board->tasks[i + 1];
    }
    board->totalTasks--;
    return 0;
}
int deleteBoard(Project *project, char *boardId){
    if (project == NULL || boardId == NULL) return 1;
    int index = -1;
    for (int i = 0; i < project->totalBoards; i++){
        if (strcmp(project->boards[i].boardId, boardId) == 0){
            index = i;
            break;
        }
    }
    if (index == -1) return 1;
    for (int i = index; i < project->totalBoards - 1; i++){
        project->boards[i] = project->boards[i + 1];
    }
    project->totalBoards--;
    return 0;
}
int updateBoard(Project *project,char * boardId){}
int updateTask(Project *project,char * boardId,char *taskId){}