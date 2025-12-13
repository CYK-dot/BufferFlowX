# 设计目标
输入startUML语法描述的状态机，输出C码状态机开发包。
提供状态机入口，通过传入开发包初始化。
状态机执行时，只支持传入事件。

# 输入设计

# 提示词工程

请你帮我设计一个基于plantUML状态图生成C代码的python脚本。
1. 约束条件为
    a. 不支持分支、并发、历史等高级功能
    b. 每个状态都必须有声明。
    c. 对于有嵌套结构的状态，必须用[*]->指明初始状态。
    d. 不支持终止状态->[*]
    e. 对嵌套事件的响应符合一般上的理解：如果子状态无法处理此事件，则上升到父节点，直到父节点为0（代表没有父节点）
    f. 状态、事件名称不允许携带非法符号，例如.$%+-*/等
2. 生成方式
    a. .c与.h文件均采用jinja2模板生成。
    b. 先生成.h，明确每个状态和事件的ID，然后生成.c文件
    c. .c文件中只包含表驱动的常量。
在动手之前，请你先浏览下我提供的示例，并向我确认思路，我确认无误后你再开始编写python脚本和jinja2模板。

示例输入UML
@startuml exampleProj

    status1: status1_comment
    status2: status2_comment

    [*] --> status1
    status1 --> status2 : event1
    status2 --> status1 : event2
    status2 --> status1 : event3/event_comment1

    state status1 {
        [*] --> sub1
        sub2: sub1_comment
        sub1 --> sub2 : event4
        sub2 --> sub1 : event5/event_comment2
    }
@enduml

示例输出.h文件，包含状态和事件ID
#ifndef __BFX_EXAMPLEPROJ_H__
#define __BFX_EXAMPLEPROJ_H__

#define EXAMPLEPROJ_STATUS1 1 // 0有特殊含义，从1开始编号
#define EXAMPLEPROJ_STATUS2 2
#define EXAMPLEPROJ_STATUS1_SUB1 3
#define EXAMPLEPROJ_STATUS1_SUB2 4

#define EXAMPLEPROJ_EVENT1 1
#define EXAMPLEPROJ_EVENT2 2
#define EXAMPLEPROJ_EVENT3 3
#define EXAMPLEPROJ_EVENT4 4
#define EXAMPLEPROJ_EVENT5 5

#endif

示例输出.c文件，只包含表驱动的常量
const BFX_FSM_TRAN_RECORD g_exampleProj_status1_TransTbl[] = {
    {EXAMPLEPROJ_EVENT1, EXAMPLEPROJ_STATUS2},
};

const BFX_FSM_TRAN_RECORD g_exampleProj_status2_TransTbl[] = {
    {EXAMPLEPROJ_EVENT2, EXAMPLEPROJ_STATUS1},
    {EXAMPLEPROJ_EVENT3, EXAMPLEPROJ_STATUS1}, ///< event_comment1
};

const BFX_FSM_TRAN_RECORD g_exampleProj_status1_sub1_TransTbl[] = {
    {EXAMPLEPROJ_EVENT4, EXAMPLEPROJ_STATUS1_SUB2},
};

const BFX_FSM_TRAN_RECORD g_exampleProj_status1_sub2_TransTbl[] = {
    {EXAMPLEPROJ_EVENT5, EXAMPLEPROJ_STATUS1_SUB1},
};

const BFX_FSM_STATE g_exampleProj_allstatus[] = {
    {
        EXAMPLEPROJ_STATUS1,
        EXAMPLEPROJ_STATUS1_SUB1,
        BFX_STATUS_FATHER_NONE,
        1, g_exampleProj_status1_TransTbl,
        BFX_exampleProj_Status1_ActionCb
    },
    {
        EXAMPLEPROJ_STATUS2,
        EXAMPLEPROJ_STATUS2,
        BFX_STATUS_FATHER_NONE,
        2, g_exampleProj_status2_TransTbl,
        BFX_exampleProj_Status2_ActionCb
    },
    {
        EXAMPLEPROJ_STATUS1_SUB1,
        EXAMPLEPROJ_STATUS1_SUB1,
        EXAMPLEPROJ_STATUS1,
        1, g_exampleProj_status1_sub1_TransTbl,
        BFX_exampleProj_Status1_Sub1_ActionCb
    },
    {
        EXAMPLEPROJ_STATUS1_SUB2,
        EXAMPLEPROJ_STATUS1_SUB2,
        EXAMPLEPROJ_STATUS1,
        1, g_exampleProj_status1_sub2_TransTbl,
        BFX_exampleProj_Status1_Sub2_ActionCb
    },
};

生成C代码表格时涉及的结构体
typedef struct tagBFX_FSM_TRAN_RECORD {
    uint8_t event;
    uint8_t nextState;
} BFX_FSM_TRAN_RECORD;

typedef struct tagBFX_FSM_ACTION_CTX {
    uint8_t stateID;
    uint8_t eventID;
    uint8_t entryType; ///< 0: entry, 1: exit, 2: action/guard
} BFX_FSM_ACTION_CTX;

typedef void (*BFX_FSM_ACTION_CALLBACK)(BFX_FSM_ACTION_CTX *ctx, void *arg, uint16_t argSize);

typedef struct tagBFX_FSM_STATE {
    uint8_t stateID;
    uint8_t defaultStateID; ///< 当该状态下没有嵌套状态时，defaultStateID = stateID
    uint8_t fatherStateID;  ///< 当该状态不为嵌套子状态时，fatherStateID = 0
    uint8_t tranTblCnt;
    BFX_FSM_TRAN_RECORD *tranTbl;
    BFX_FSM_ACTION_CALLBACK actionTbl;
} BFX_FSM_STATE;