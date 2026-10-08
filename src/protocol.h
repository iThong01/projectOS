#ifndef PROTOCOL_H
#define PROTOCOL_H


#define COMMAND_ADD_BOARD "Add_Board"
#define COMMAND_ADD_TASK "Add_Task"
#define COMMAND_GET_BOARDS "Get_Boards" 
#define COMMAND_GET_TASK "Get_Task"  //not done
#define COMMAND_UPDATE_BOARD "Update_Board"  //not done
#define COMMAND_UPDATE_TASK "Update_Task"  //not done
#define COMMAND_DELETE_TASK "Delete_Task"
#define COMMAND_DELETE_BOARD "Delete_Board"
#define COMMAND_SEPARATOR ","
#define COMMAND_UNKNOWN "UNKNOWN"

typedef enum {
    // --- 0xXX: Success & Basic ---
    STATUS_OK = 0,
    STATUS_FAIL = 1,
    STATUS_UNKNOWN = 3,

    // --- 2xXX: Client/Request Errors (200+) ---
    STATUS_INVALID_ARGUMENTS = 200,
    STATUS_BOARD_NOT_FOUND = 201,
    STATUS_EMPTY_BOARD = 202,
    STATUS_TASK_NOT_FOUND = 203,
    STATUS_EMPTY_TASK = 204,
    STATUS_BOARD_NOT_EMPTY = 205,

    // --- 5xXX: Server/System Errors (500+) ---
    STATUS_SAVE_BOARD_FAIL = 500,
    STATUS_ADD_BOARD_FAIL = 501,
    STATUS_SAVE_TASK_FAIL = 502,
    STATUS_ADD_TASK_FAIL = 503,
} ResponseStatus;

#endif