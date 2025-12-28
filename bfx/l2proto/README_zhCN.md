# L2协议组件 (bfx_l2proto)

## 简介

L2协议组件（bfx_l2proto）是BufferFlowX框架中的二层协议实现，主要用于为SPI/UART等串行通信接口提供数据帧的成帧与同步功能，以及差错控制机制。该组件提供了一套可配置的协议栈，允许用户根据具体应用场景定制帧格式。

## 主要功能

- **可配置帧格式**：支持定制帧头、同步序列、帧长度、数据字段和FCS（帧校验序列）
- **数据成帧与解帧**：提供数据的编码（成帧）和解码（解帧）功能
- **差错检测**：通过FCS（帧校验序列）实现数据完整性校验
- **字节同步**：通过预设的前导码实现数据同步
- **用户自定义字段**：支持在帧头中嵌入用户特定信息

## 核心概念

### 协议帧结构

L2协议的帧结构如下：

```
|| 前导同步序列 (Preamble) || 用户字段/长度字段 (usr/len) || 数据 (data) || 帧校验序列 (FCS) ||
```

- **前导同步序列**：用于同步接收端，使用固定字节（默认为0xAA）
- **头字段**：包含用户自定义字段和数据长度信息
- **数据字段**：实际传输的数据
- **FCS字段**：用于差错检测的校验序列

### 关键数据结构

- `BFX_PROTO_L2_DESC`：协议描述符，定义协议参数
- `BFX_PROTO_L2_RX_BUFFER`：接收缓冲区结构
- `BFX_PROTO_L2_PKT`：数据包载荷结构
- `BFX_PROTO_L2_EVENT`：协议事件枚举

## 目录结构

```
bfx/l2proto/
├── bfx_l2proto.h     # 头文件，包含接口声明
├── bfx_l2proto.c     # 实现文件，包含协议实现
└── design.md         # 设计文档
```

## 快速开始

### 1. 配置协议描述符

```c
BFX_PROTO_L2_DESC desc = {
    .fcsCalc = your_fcs_calc_function,  // FCS计算函数
    .hton = your_hton_function,          // 主机字节序转网络字节序函数
    .ntoh = your_ntoh_function,          // 网络字节序转主机字节序函数
    .preambleByteCnt = 4,               // 前导码字节数
    .headByteCnt = 2,                   // 头部字节数
    .lenBitCnt = 12,                    // 长度字段位数
    .fcsByteCnt = 2                     // FCS字节数
};
```

### 2. 数据编码（成帧）

```c
BFX_PROTO_L2_PKT payload = {
    .data = your_data,
    .dataLen = data_length,
    .usr = user_field_value
};

uint8_t output_buffer[256];
uint16_t encoded_len = BFX_ProtoL2Encode(&desc, &payload, output_buffer, sizeof(output_buffer));
```

### 3. 数据解码（解帧）

```c
BFX_PROTO_L2_RX_BUFFER rx_buffer;
uint8_t rx_buf[256];
BFX_ProtoL2SetupRxBuffer(&rx_buffer, rx_buf, sizeof(rx_buf));

// 逐字节处理接收数据
for (int i = 0; i < received_data_len; i++) {
    BFX_PROTO_L2_EVENT event = BFX_ProtoL2Decode(&desc, received_byte[i], &rx_buffer, &decoded_payload);
    
    switch (event) {
        case BFX_PROTOL2_EVENT_ENCODED_PKT:
            // 成功接收到完整数据包
            process_decoded_packet(&decoded_payload);
            break;
        case BFX_PROTOL2_EVENT_DROP_SYNC_ERROR:
            // 同步错误
            break;
        case BFX_PROTOL2_EVENT_DROP_FCS_ERROR:
            // FCS校验错误
            break;
        default:
            // 其他事件
            break;
    }
}
```

## 详细使用指南

### 协议参数配置

- `preambleByteCnt`：前导码字节数，用于接收端同步
- `headByteCnt`：头部字节数，包含用户字段和长度信息
- `lenBitCnt`：数据长度字段的位数，决定了最大可传输数据长度
- `fcsByteCnt`：FCS校验字段字节数
- `fcsCalc`：FCS计算回调函数
- `hton/ntoh`：字节序转换回调函数

### 内存管理

- 编码时需要提供足够大的输出缓冲区
- 解码时需要提供足够大的接收缓冲区，至少为 `maxDataLen + 2 * fcsByteCnt`

### 错误处理

组件提供了多种错误事件类型：

- `BFX_PROTOL2_EVENT_PARAM_ERROR`：参数错误
- `BFX_PROTOL2_EVENT_DROP_SYNC_ERROR`：同步错误
- `BFX_PROTOL2_EVENT_DROP_FCS_ERROR`：FCS校验错误

## 高级特性

### 自定义FCS算法

用户可以实现自己的FCS计算函数，支持多种校验算法如CRC16、CRC32等。

### 字节序转换

支持网络字节序和主机字节序之间的转换，确保跨平台兼容性。

### 可配置参数

协议的各个部分都可以根据应用需求进行配置，提供了高度的灵活性。

## 最佳实践

1. **参数检查**：在使用协议前，确保配置参数符合实际需求
2. **缓冲区管理**：合理分配接收缓冲区大小，避免溢出
3. **错误处理**：正确处理各种协议事件，特别是错误事件
4. **性能优化**：选择高效的FCS算法和字节序转换函数

## 常见问题解答

### Q: 为什么前导码使用固定字节0xAA？

A: 0xAA（10101010）在串行通信中具有良好的同步特性，其交替的高低电平模式有助于接收端的时钟恢复和同步。

### Q: 如何确定合适的FCS长度？

A: FCS长度取决于对数据完整性的要求。较短的FCS（如CRC-16）适合较短的数据包和低误码率环境，较长的FCS（如CRC-32）适合较长的数据包和高可靠性要求。

## 故障排除

### 数据包丢失

- 检查前导码配置是否正确
- 确认接收缓冲区大小是否足够
- 验证FCS计算函数是否正确实现

### 同步错误

- 确认发送端和接收端的协议参数配置一致
- 检查物理层通信是否稳定
- 验证字节序转换函数是否正确

### 校验错误

- 确认FCS计算算法在发送端和接收端一致
- 检查数据传输过程中是否有干扰
- 验证字节序转换是否正确执行