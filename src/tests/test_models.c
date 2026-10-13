#include <stdio.h>
#include <stdlib.h>
#include "models.h"
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h> //mkdir
#include <unistd.h> //rmdir

#include "models.h"

static Project *createTestProject(){
    Project *project = calloc(1, sizeof(Project));
    assert(project != NULL);
    return project;
}
void testAddBoardSuccess(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);
    assert(project->totalBoards == 1);
    assert(isEmptyOrSpace(project->boards[0].boardId) == 0);
    assert(strcmp(project->boards[0].boardTitle, "Board 1") == 0);
    printf("PASS testAddBoardSuccess\n");
    free(project);
}
void testAddTaskSuccess(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);
    char *boardId = project->boards[0].boardId;

    assert(addTask(project, boardId, "Task 1") == 0);
    assert(project->boards[0].totalTasks == 1);
    assert(isEmptyOrSpace(project->boards[0].tasks[0].taskId) == 0);
    assert(strcmp(project->boards[0].tasks[0].taskTitle, "Task 1") == 0);
    assert(project->boards[0].tasks[0].isFinish == 0);

    printf("PASS testAddTaskSuccess\n");
    free(project);
}

void testSaveBoard(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);
    assert(saveBoard(project, BOARD_FILENAME) == 0);

    FILE *file = fopen(BOARD_FILENAME, "r");
    assert(file != NULL);
    char line[1024];
    assert(fgets(line, sizeof(line), file) != NULL);
    assert(strcmp(line, "1,Board 1\n") == 0);
    assert(fgets(line, sizeof(line), file) == NULL);

    fclose(file);
    printf("PASS testSaveBoard\n");
    free(project);
}
void testSaveTask(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);
    char *boardId = project->boards[0].boardId;
    assert(addTask(project, boardId, "Task 1") == 0);
    assert(saveAllTasks(project, TASK_FILENAME) == 0);

    FILE *file = fopen(TASK_FILENAME, "r");
    assert(file != NULL);
    char line[1024];
    assert(fgets(line, sizeof(line), file) != NULL);
    assert(strcmp(line, "1,1,Task 1,0\n") == 0);
    assert(fgets(line, sizeof(line), file) == NULL);

    fclose(file);
    printf("PASS testSaveTask\n");
    free(project);
}

void testReadBoard(){
    Project *source = createTestProject();
    assert(addBoard(source, "Board 1") == 0);
    free(source);

    Project *loaded = createTestProject();
    assert(readBoard(loaded, BOARD_FILENAME) == 0);
    assert(loaded->totalBoards == 1);
    assert(strcmp(loaded->boards[0].boardId, "1") == 0);
    assert(strcmp(loaded->boards[0].boardTitle, "Board 1") == 0);
    printf("PASS testReadBoard\n");
    free(loaded);
}
void testReadTask(){
    Project *source = createTestProject();
    assert(addBoard(source, "Board 1") == 0);
    char *boardId = source->boards[0].boardId;
    assert(addTask(source, boardId, "Task 1") == 0);

    free(source);

    Project *loaded = createTestProject();
    assert(readBoard(loaded, BOARD_FILENAME) == 0);
    assert(readAllTasks(loaded, TASK_FILENAME) == 0);
    assert(loaded->boards[0].totalTasks == 1);
    assert(strcmp(loaded->boards[0].tasks[0].taskId, "1") == 0);
    assert(strcmp(loaded->boards[0].tasks[0].taskTitle, "Task 1") == 0);
    assert(loaded->boards[0].tasks[0].isFinish == 0);

    printf("PASS testReadTask\n");
    free(loaded);
}

void testLoadProject(){
    Project *source = createTestProject();
    assert(addBoard(source, "Board 1") == 0);
    char *boardId = source->boards[0].boardId;
    assert(addTask(source, boardId, "Task 1") == 0);
    free(source);

    Project *loaded = createTestProject();
    assert(loadProject(loaded, BOARD_FILENAME, TASK_FILENAME) == 0);
    assert(loaded->totalBoards == 1);
    assert(loaded->boards[0].totalTasks == 1);
    assert(strcmp(loaded->boards[0].boardTitle, "Board 1") == 0);
    assert(strcmp(loaded->boards[0].tasks[0].taskTitle, "Task 1") == 0);

    printf("PASS testLoadProject\n");
    free(loaded);
}

void testAddTaskBoardNotFound(){
    Project *project = createTestProject();
    assert(addTask(project, "999", "Task 1") == 1);
    assert(project->totalBoards == 0);
    printf("PASS testAddTaskBoardNotFound\n");
    free(project);
}
void testAddTaskEmptyTitle(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);
    char *boardId = project->boards[0].boardId;
    assert(addTask(project, boardId, "") == 1);
    assert(project->boards[0].totalTasks == 0);

    printf("PASS testAddTaskEmptyTitle\n");
    free(project);
}
void testAddTaskLongTitle(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);
    char longTitle[TASK_TITLE_SIZE + 1];
    memset(longTitle, 'A', TASK_TITLE_SIZE);
    longTitle[TASK_TITLE_SIZE] = '\0';

    char *boardId = project->boards[0].boardId;
    assert(addTask(project, boardId, longTitle) == 1);
    assert(project->boards[0].totalTasks == 0);
    printf("PASS testAddTaskLongTitle\n");
    free(project);
}
void testAddTaskFull(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    board->totalTasks = MAX_TASK;
    assert(addTask(project, board->boardId, "New Task") == 1);
    assert(board->totalTasks == MAX_TASK);

    printf("PASS testAddTaskFull\n");
    free(project);
}

void testUpdateBoardSuccess(){
    Project *project = createTestProject();
    assert(addBoard(project, "Old Board") == 0);

    char *boardId = project->boards[0].boardId;
    assert(updateBoard(project, boardId, "New Board") == 0);
    assert(project->totalBoards == 1);
    assert(strcmp(project->boards[0].boardTitle, "New Board") == 0);

    printf("PASS testUpdateBoardSuccess\n");
    free(project);
}
void testUpdateBoardNotFound(){
    Project *project = createTestProject();
    assert(addBoard(project, "Old Board") == 0);
    assert(updateBoard(project, "999", "New Board") == 1);
    assert(strcmp(project->boards[0].boardTitle, "Old Board") == 0);

    printf("PASS testUpdateBoardNotFound\n");
    free(project);
}
void testUpdateBoardEmptyTitle(){
    Project *project = createTestProject();
    assert(addBoard(project, "Old Board") == 0);

    char *boardId = project->boards[0].boardId;

    assert(updateBoard(project, boardId, "") == 1);
    assert(strcmp(project->boards[0].boardTitle, "Old Board") == 0);

    printf("PASS testUpdateBoardEmptyTitle\n");
    free(project);
}
void testUpdateBoardLongTitle(){
    Project *project = createTestProject();
    assert(addBoard(project, "Old Board") == 0);

    char longTitle[BOARD_TITLE_SIZE + 1];
    memset(longTitle, 'A', BOARD_TITLE_SIZE);
    longTitle[BOARD_TITLE_SIZE] = '\0';

    char *boardId = project->boards[0].boardId;
    assert(updateBoard(project, boardId, longTitle) == 1);
    assert(strcmp(project->boards[0].boardTitle, "Old Board") == 0);

    printf("PASS testUpdateBoardLongTitle\n");
    free(project);
}

void testUpdateTaskSuccess(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    assert(addTask(project, board->boardId, "Old Task") == 0);

    char *taskId = board->tasks[0].taskId;
    assert(updateTask(project, board->boardId, taskId, "New Task") == 0);
    assert(board->totalTasks == 1);
    assert(strcmp(board->tasks[0].taskTitle, "New Task") == 0);

    printf("PASS testUpdateTaskSuccess\n");
    free(project);
}
void testUpdateTaskNotFound(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    assert(addTask(project, board->boardId, "Old Task") == 0);

    assert(updateTask(project, board->boardId, "999", "New Task") == 1);
    assert(strcmp(board->tasks[0].taskTitle, "Old Task") == 0);

    printf("PASS testUpdateTaskNotFound\n");
    free(project);
}
void testUpdateTaskEmptyTitle(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    assert(addTask(project, board->boardId, "Old Task") == 0);

    char *taskId = board->tasks[0].taskId;
    assert(updateTask(project, board->boardId, taskId, "") == 1);
    assert(strcmp(board->tasks[0].taskTitle, "Old Task") == 0);

    printf("PASS testUpdateTaskEmptyTitle\n");
    free(project);
}
void testUpdateTaskLongTitle(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    assert(addTask(project, board->boardId, "Old Task") == 0);

    char longTitle[TASK_TITLE_SIZE + 1];
    memset(longTitle, 'A', TASK_TITLE_SIZE);
    longTitle[TASK_TITLE_SIZE] = '\0';

    char *taskId = board->tasks[0].taskId;
    assert(updateTask(project, board->boardId, taskId, longTitle) == 1);
    assert(strcmp(board->tasks[0].taskTitle, "Old Task") == 0);

    printf("PASS testUpdateTaskLongTitle\n");
    free(project);
}

void testDeleteBoardSuccess(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);
    assert(addBoard(project, "Board 2") == 0);
    char boardId[BOARDID_SIZE];
    strcpy(boardId, project->boards[0].boardId);

    assert(deleteBoard(project, boardId) == 0);
    assert(project->totalBoards == 1);
    assert(strcmp(project->boards[0].boardTitle, "Board 2") == 0);
    assert(hasBoard(project, boardId) == NULL);

    printf("PASS testDeleteBoardSuccess\n");
    free(project);
}
void testDeleteBoardNotFound(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);
    assert(deleteBoard(project, "999") == 1);
    assert(project->totalBoards == 1);
    assert(strcmp(project->boards[0].boardTitle, "Board 1") == 0);
    printf("PASS testDeleteBoardNotFound\n");
    free(project);
}
void testDeleteBoardWithTasks(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    char boardId[BOARDID_SIZE];
    strcpy(boardId, project->boards[0].boardId);
    assert(addTask(project, boardId, "Task 1") == 0);
    assert(deleteBoard(project, boardId) == 0);
    assert(project->totalBoards == 0);
    free(project);

    Project *loaded = createTestProject();
    assert(loadProject(loaded, BOARD_FILENAME, TASK_FILENAME) == 0);
    assert(loaded->totalBoards == 0);
    printf("PASS testDeleteBoardWithTasks\n");
    free(loaded);
}

void testDeleteTaskSuccess(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    assert(addTask(project, board->boardId, "Task 1") == 0);
    assert(addTask(project, board->boardId, "Task 2") == 0);

    char taskId[TASKID_SIZE];
    strcpy(taskId, board->tasks[0].taskId);
    assert(deleteTask(project, board->boardId, taskId) == 0);
    assert(board->totalTasks == 1);
    assert(strcmp(board->tasks[0].taskTitle, "Task 2") == 0);
    assert(hasTask(project, board->boardId, taskId) == NULL);

    printf("PASS testDeleteTaskSuccess\n");
    free(project);
}
void testDeleteTaskNotFound(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    assert(addTask(project, board->boardId, "Task 1") == 0);
    assert(deleteTask(project, board->boardId, "999") == 1);
    assert(board->totalTasks == 1);
    assert(strcmp(board->tasks[0].taskTitle, "Task 1") == 0);

    printf("PASS testDeleteTaskNotFound\n");
    free(project);
}
void testDeleteTaskBoardNotFound(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    assert(addTask(project, board->boardId, "Task 1") == 0);
    assert(deleteTask(project, "999", board->tasks[0].taskId) == 1);
    assert(board->totalTasks == 1);
    assert(strcmp(board->tasks[0].taskTitle, "Task 1") == 0);

    printf("PASS testDeleteTaskBoardNotFound\n");
    free(project);
}

void testDeleteTaskPersistence(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    assert(addTask(project, board->boardId, "Task 1") == 0);
    assert(addTask(project, board->boardId, "Task 2") == 0);

    char deletedId[TASKID_SIZE];
    strcpy(deletedId, board->tasks[0].taskId);
    assert(deleteTask(project, board->boardId, deletedId) == 0);
    free(project);

    Project *loaded = createTestProject();
    assert(loadProject(loaded, BOARD_FILENAME, TASK_FILENAME) == 0);
    assert(loaded->totalBoards == 1);
    assert(loaded->boards[0].totalTasks == 1);
    assert(strcmp(loaded->boards[0].tasks[0].taskTitle, "Task 2") == 0);
    assert(hasTask(loaded, loaded->boards[0].boardId, deletedId) == NULL);

    printf("PASS testDeleteTaskPersistence\n");
    free(loaded);
}
void testDeleteBoardTaskPersistence(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    char boardId[BOARDID_SIZE];
    strcpy(boardId, project->boards[0].boardId);
    assert(addTask(project, boardId, "Task 1") == 0);
    assert(deleteBoard(project, boardId) == 0);
    assert(project->totalBoards == 0);

    FILE *file = fopen(TASK_FILENAME, "r");
    assert(file != NULL);
    assert(fgetc(file) == EOF);
    fclose(file);

    printf("PASS testDeleteBoardTaskPersistence\n");
    free(project);
}

void testUpdateBoardRollback(){
    Project *project = createTestProject();
    assert(addBoard(project, "Old Board") == 0);

    char *boardId = project->boards[0].boardId;
    assert(remove(BOARD_FILENAME) == 0);
    assert(mkdir(BOARD_FILENAME, 0700) == 0);
    assert(updateBoard(project, boardId, "New Board") == 1);
    assert(strcmp(project->boards[0].boardTitle, "Old Board") == 0);
    assert(project->totalBoards == 1);
    assert(rmdir(BOARD_FILENAME) == 0);

    printf("PASS testUpdateBoardRollback\n");
    free(project);
}
void testUpdateTaskRollback(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    assert(addTask(project, board->boardId, "Old Task") == 0);

    char *taskId = board->tasks[0].taskId;
    assert(remove(TASK_FILENAME) == 0);
    assert(mkdir(TASK_FILENAME, 0700) == 0);
    assert(updateTask(project, board->boardId,taskId, "New Task") == 1);
    assert(strcmp(board->tasks[0].taskTitle, "Old Task") == 0);
    assert(board->totalTasks == 1);
    assert(rmdir(TASK_FILENAME) == 0);

    printf("PASS testUpdateTaskRollback\n");
    free(project);
}
void testDeleteTaskRollback(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    assert(addTask(project, board->boardId, "Task 1") == 0);
    assert(addTask(project, board->boardId, "Task 2") == 0);
    assert(addTask(project, board->boardId, "Task 3") == 0);

    char taskId[TASKID_SIZE];
    strcpy(taskId, board->tasks[1].taskId);
    assert(remove(TASK_FILENAME) == 0);
    assert(mkdir(TASK_FILENAME, 0700) == 0);
    assert(deleteTask(project, board->boardId, taskId) == 1);
    assert(board->totalTasks == 3);
    assert(strcmp(board->tasks[0].taskTitle, "Task 1") == 0);
    assert(strcmp(board->tasks[1].taskTitle, "Task 2") == 0);
    assert(strcmp(board->tasks[2].taskTitle, "Task 3") == 0);
    assert(strcmp(board->tasks[1].taskId, taskId) == 0);
    assert(rmdir(TASK_FILENAME) == 0);

    printf("PASS testDeleteTaskRollback\n");
    free(project);
}
void testDeleteBoardRollback(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);
    assert(addBoard(project, "Board 2") == 0);
    assert(addBoard(project, "Board 3") == 0);

    Board *board = &project->boards[1];
    assert(addTask(project, board->boardId, "Task B") == 0);

    char boardId[BOARDID_SIZE];
    strcpy(boardId, board->boardId);
    assert(remove(BOARD_FILENAME) == 0);
    assert(mkdir(BOARD_FILENAME, 0700) == 0);
    assert(deleteBoard(project, boardId) == 1);
    assert(project->totalBoards == 3);
    assert(strcmp(project->boards[0].boardTitle, "Board 1") == 0);
    assert(strcmp(project->boards[1].boardTitle, "Board 2") == 0);
    assert(strcmp(project->boards[2].boardTitle, "Board 3") == 0);
    assert(strcmp(project->boards[1].boardId, boardId) == 0);
    assert(project->boards[1].totalTasks == 1);
    assert(strcmp(project->boards[1].tasks[0].taskTitle, "Task B") == 0);
    assert(rmdir(BOARD_FILENAME) == 0);

    printf("PASS testDeleteBoardRollback\n");
    free(project);
}
void testAddBoardRollback(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);
    assert(remove(BOARD_FILENAME) == 0);
    assert(mkdir(BOARD_FILENAME, 0700) == 0);
    assert(addBoard(project, "Board 2") == 1);
    assert(project->totalBoards == 1);
    assert(strcmp(project->boards[0].boardTitle,"Board 1") == 0);
    assert(hasBoard(project, "2") == NULL);
    assert(rmdir(BOARD_FILENAME) == 0);

    printf("PASS testAddBoardRollback\n");
    free(project);
}
void testAddTaskRollback(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);

    Board *board = &project->boards[0];
    assert(addTask(project, board->boardId, "Task 1") == 0);

    char originalTaskId[TASKID_SIZE];
    strcpy(originalTaskId, board->tasks[0].taskId);
    assert(remove(TASK_FILENAME) == 0);
    assert(mkdir(TASK_FILENAME, 0700) == 0);
    assert(addTask(project, board->boardId, "Task 2") == 1);
    assert(board->totalTasks == 1);
    assert(strcmp(board->tasks[0].taskId,originalTaskId) == 0);
    assert(strcmp(board->tasks[0].taskTitle,"Task 1") == 0);
    assert(rmdir(TASK_FILENAME) == 0);

    printf("PASS testAddTaskRollback\n");
    free(project);
}

void testDeleteBoardPartialSaveFailure(){
    Project *project = createTestProject();
    assert(addBoard(project, "Board 1") == 0);
    assert(addBoard(project, "Board 2") == 0);

    char boardId[BOARDID_SIZE];
    strcpy(boardId, project->boards[0].boardId);
    assert(addTask(project, boardId, "Task 1") == 0);
    assert(remove(TASK_FILENAME) == 0);
    assert(mkdir(TASK_FILENAME, 0700) == 0);
    assert(deleteBoard(project, boardId) == 1);
    assert(project->totalBoards == 2);
    assert(strcmp(project->boards[0].boardTitle, "Board 1") == 0);
    assert(strcmp(project->boards[1].boardTitle, "Board 2") == 0);
    assert(project->boards[0].totalTasks == 1);
    assert(strcmp(project->boards[0].tasks[0].taskTitle, "Task 1") == 0);

    Project *loaded = createTestProject();
    assert(readBoard(loaded, BOARD_FILENAME) == 0);
    assert(loaded->totalBoards == 2);
    assert(strcmp(loaded->boards[0].boardTitle, "Board 1") == 0);
    assert(strcmp(loaded->boards[1].boardTitle, "Board 2") == 0);

    free(loaded);
    assert(rmdir(TASK_FILENAME) == 0);

    printf("PASS testDeleteBoardPartialSaveFailure\n");
    free(project);
}
int main(){
    printf("Start testing all models\n\n");
    {
    printf("Test Positive\n");
    testAddBoardSuccess();
    testAddTaskSuccess();
    testSaveBoard();
    testSaveTask();
    testReadBoard();
    testReadTask();
    testLoadProject();
    }
    {
    printf("\nTest Board Neg\n");
    testAddTaskBoardNotFound();
    testAddTaskEmptyTitle();
    testAddTaskLongTitle();
    testAddTaskFull();
    }
    {
    printf("\nTest Update Board\n");
    testUpdateBoardSuccess();
    testUpdateBoardNotFound();
    testUpdateBoardEmptyTitle();
    testUpdateBoardLongTitle();
    }
    {
    printf("\nTest Update Task\n");
    testUpdateTaskSuccess();
    testUpdateTaskNotFound();
    testUpdateTaskEmptyTitle();
    testUpdateTaskLongTitle();
    }
    {
    printf("\nTest Del Board\n");
    testDeleteBoardSuccess();
    testDeleteBoardNotFound();
    testDeleteBoardWithTasks();
    }
    printf("\nTest Del Task\n");
    testDeleteTaskSuccess();
    testDeleteTaskNotFound();
    testDeleteTaskBoardNotFound();

    printf("\nTest after del update file yet\n");
    testDeleteTaskPersistence();
    testDeleteBoardTaskPersistence();

    printf("\nroolback\n");
    testUpdateBoardRollback();
    testUpdateTaskRollback();
    testDeleteTaskRollback();
    testDeleteBoardRollback();
    testAddBoardRollback();
    testAddTaskRollback();

    testDeleteBoardPartialSaveFailure();
    printf("\nAll addBoard tests passed!\n");
    return 0;
}
