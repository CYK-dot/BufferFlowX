# BufferFlowX
A collection of commonly used MCU data structures, primarily focusing on queues.
[中文文档](README_CN.md)</br>
![codecov](https://codecov.io/gh/CYK-dot/BufferFlowX/branch/main/graph/badge.svg)

# DFIFO
Double Buffer, also known as AB Buffer, eliminates mutual exclusion by alternating the use of two buffers, enabling simultaneous read and write operations.</br>
For example, DFIFO can be used to drive a display, where the renderer writes data into one buffer while the driver reads from the other buffer and displays it on the screen, both operating concurrently.</br>

## How to use it?
DFIFO is designed for asynchronous zero-copy operations. Both read and write operations always follow an Acquire-then-Complete pattern.</br>
Asynchronous zero-copy was initially designed for DMA, allowing data acquisition before DMA starts and data submission in the DMA interrupt, avoiding secondary copying.</br>
Of course, its usage is not limited to DMA; it applies to any scenario requiring asynchronous or zero-copy operations.</br>
What if you don't need asynchronous or zero-copy? Simply wrap the functions so that Acquire and Complete are called within the same context.</br>
```c
/* Synchronous data transmission */
void ExampleSyncSend(uint8_t *mem, BFX_DFIFO_CB *cb, uint8_t *dataToSend, uint16_t dataLen)
{
    if (dataLen > cb->sliceSize) {
        return;
    }
    uint8_t *acquiredMem = BFX_DfifoSendAcquire(mem, cb);
    if (acquiredMem == NULL) {
        return;
    }
    memcpy(acquiredMem, dataToSend, dataLen);
    BFX_DfifoSendComplete(cb);
}
```

## How it works?
Each buffer has four states, always transitioning in the cycle: free → wr → ocp → rd → free.</br>
The essence of the double buffer lies in the coordination between the two state machines. Part of its truth table is shown below.</br>
| (A state, B state) | Call function | Result | Function return value | Explanation |
| --- | --- | --- | --- | --- |
| (free, free) | SendAcquire | (wr,free) | BufferA | When both are free, allocate memory, prioritizing A |
| (wr, free) | SendComplete | (ocp,free) | None | Transmission complete, switch to ocp state |
| (ocp, free) | SendAcquire | (ocp,wr) | BufferB | A has data, B is unoccupied, so write to B |
| (ocp, free) | RecvAcquire | (rd,free) | BufferA | Only A has data, so return A to the user |
| (free, ocp) | RecvAcquire | (free,rd) | BufferB | Only B has data, so return B to the user |
| (wr, ocp) | RecvAcquire | (wr,rd) | BufferB | Producer occupies A, does not interfere with consumer reading from B |
| (wr,free) | SendAcquire | (wr,free) | NULL | Producer cannot write to A while it is occupied |
| (rd, ocp) | RecvAcquire | (rd, ocp) | NULL | Consumer cannot read from B while it is occupied |

There is a special case: when the producer continuously writes data and the consumer never reads, both A and B end up in the ocp state.</br>
To ensure the consumer always reads the oldest data, the double buffer additionally stores a variable called lastBuffer, indicating which buffer holds the newest data.</br>
| lastBuffer | Call function | Function return value | Explanation |
| --- | --- | --- | --- |
| A | RecvAcquire | BufferB | A has the newest data, so B holds the oldest data |
| B | RecvAcquire | BufferA | B has the newest data, so A holds the oldest data |
| A | SendAcquire | BufferB | Request to write data, so it will overwrite the oldest data. After the function call, the state changes from (ocp,ocp) to (ocp,wr) |
| B | SendAcquire | BufferA | State changes from (ocp,ocp) to (wr,ocp) |
