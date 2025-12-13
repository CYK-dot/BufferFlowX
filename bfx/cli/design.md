# functions
## BFX_CliRawMatch
paramStore的复用：
1. 在计算fmt的token总数时，临时存放fmt的token索引，但并不使用
2. 存放cmd的token索引。
3. 存放cmd的paramToken索引。
其中，1和23不可能同时发生，因此复用是没问题的。
注意到，param总数总是<=token总数，因此在遍历所有token时，修改内容总不会超过当前迭代器，因此23也是可以复用的。

@startuml
' @state_id:Idle=0x1001
' @state_id:Running=0x1002
' @event_id:start=0x2001
' @event_id:stop=0x2002

[*] --> Idle
Idle --> Running : start
Running --> Idle : stop
@enduml