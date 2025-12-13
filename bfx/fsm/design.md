# 设计目标
输入startUML语法描述的状态机，输出C码状态机开发包。
提供状态机入口，通过传入开发包初始化。
状态机执行时，只支持传入事件。

# 输入设计
## StartUML定制
@startuml 组件名称
    ' 支持状态注释，生成C码中的/** */
    Status1: 状态1
    Status2: 状态2
    Status3: 状态3
    Status1: 初始状态
    ' 支持基本的状态转换描述
    Status1 --> Status2 : event1
    Status2 --> Status3 : event2
    Status3 --> Status1 : event3
    Status1 --> Status2 : event4
    ' 支持嵌套状态（todo）
    state Status1 {
        Sub1: 状态1_1
        Sub2: 状态1_2
        Sub1 --> Sub2 : event5
        state Sub1 {
            Sub1_1: 状态1_1_1
            Sub1_2: 状态1_1_2
            Sub1_1 --> Sub1_2 : event6
        }
    }
@enduml

## StartUML解析和生成机制
脚本分为三步，暂不考虑增量编译造成的问题，每次都是全量生成。
1. 剔除注释
2. 生成ID
3. 生成代码

生成代码的两种方案：
1. 表驱动：通过遍历状态转移表得知下一步怎么操作
2. 展开：直接把状态转移展开成switch-case，放到生成代码中。最终代码量大，效率也没见得有多高。
最终选择采用表驱动，表为const，放到flash中避免占用RAM
表结构
```c
typedef struct tagBFX_FSM_TRAN_RECORD {
    uint8_t event;
    uint8_t nextState;
} BFX_FSM_TRAN_RECORD;

typedef struct tagBFX_FSM_ACTION_CTX {
    uint8_t stateID;
    uint8_t eventID;
    uint8_t entryType;
} BFX_FSM_ACTION_CTX;

typedef void (*BFX_FSM_ACTION_CALLBACK)(BFX_FSM_ACTION_CTX *ctx, void *arg, uint16_t argSize);

typedef struct tagBFX_FSM_STATE {
    BFX_FSM_TRAN_RECORD *tranTbl;
    BFX_FSM_ACTION_CALLBACK *actionTbl;
    uint8_t tranTblCnt;
    uint8_t actionTblCnt;
    uint8_t stateID;
    uint8_t fatherStateID;
} BFX_FSM_STATE;
```
示例
```c
const BFX_FSM_TRAN_RECORD g_mainState1TranTbl[] = {
    {MAIN_EVENT1, MAIN_STATE2},
    {MAIN_EVENT2, MAIN_STATE3},
    {MAIN_EVENT3, MAIN_STATE1},
    {MAIN_EVENT4, MAIN_STATE2},
};
const BFX_FSM_STATE g_mainState1Handle = {
    .tranTbl = g_mainState1TranTbl,
    .tranTblCnt = sizeof(g_mainState1TranTbl) / sizeof(g_mainState1TranTbl[0]),
    .stateID = MAIN_STATE1,
    .fatherStateID = MAIN,
}
```