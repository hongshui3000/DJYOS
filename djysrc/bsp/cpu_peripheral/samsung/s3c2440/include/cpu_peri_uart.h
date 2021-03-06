//----------------------------------------------------
//Copyright (C), 2004-2009,  lst.
//版权所有 (C), 2004-2009,   lst.
//所属模块: uart驱动程序
//作者：lst
//版本：V1.0.1
//文件描述: 驱动44b0的uart
//其他说明:
//修订历史:
//2. ...
//1. 日期: 2009-03-10
//   作者: lst
//   新版本号: V1.0.0
//   修改说明: 原始版本
//------------------------------------------------------
#ifndef _UART_H_
#define _UART_H_

#ifdef __cplusplus
extern "C" {
#endif

//UTRSTAT状态位定义
#define UartTXShiftEmpty    (1<<2)  //1:发送移位寄存器空,0:非空
#define UartTxBufEmpty      (1<<1)  //1:发送缓冲区空,0:非空
#define UartRxBufNotEmpty   (1<<0)  //1:接受缓冲区有数据,0:无数据
//UFCON FIFO控制寄存器
#define cn_tx_fifo_reset    (1<<2)  //写1复位发送fifo,读总是0
#define cn_rx_fifo_reset    (1<<1)  //写1复位发送fifo,读总是0
//UFSTAT FIFO状态寄存器
#define cn_rx_over          (1<<6)  //1:接收fifo溢出,0:正常

//通信状态变量各位定义
#define cn_ser_tx_busy         (1<<0)
#define cn_ser_rxbuf_empty     (1<<1)
#define cn_ser_receive_over    (1<<2)

//中断控制位，在2410的中断控制器中，每个串口只分配了一个中断，具体中断原因是通过
//查询 SUBSRCPND 寄存器相应的位来得到的，控制 INTSUBMSK 中相应位可独立地禁止
//接收/发送中断。
#define BIT_SUB_ERR2    (0x1<<8)
#define BIT_SUB_TXD2    (0x1<<7)
#define BIT_SUB_RXD2    (0x1<<6)
#define BIT_SUB_ERR1    (0x1<<5)
#define BIT_SUB_TXD1    (0x1<<4)
#define BIT_SUB_RXD1    (0x1<<3)
#define BIT_SUB_ERR0    (0x1<<2)
#define BIT_SUB_TXD0    (0x1<<1)
#define BIT_SUB_RXD0    (0x1<<0)
#define BIT_ALL_UART    (0x1ff)

//串口状态控制结构
struct uart_UCB
{
    struct ring_buf send_ring_buf;           //环形发送缓冲区.
    struct ring_buf recv_ring_buf;           //环形接收缓冲区.

    struct semaphore_LCB *send_buf_semp;     //发送缓冲区锁
    struct semaphore_LCB *recv_buf_semp;     //接收缓冲区锁

    u16 evtt_right_write;      //设备右手被写后弹出的事件类型
    u16 evtt_left_write;       //设备左手被写后弹出的事件类型
    u16 recv_trigger_level;    //串口接收触发水平,即收到多少数据时弹出事件
    u32 timeout;               //未到触发水平,持续设定时间内没有收到数据,
                                    //也弹出事件.
    uint16_t send_trigger_level;    //发送同步触发水平，当发送环形缓冲区满，将
                                    //用send_buf_semp阻塞当前线程，直到发送缓冲
                                    //区数据量降至send_trigger_level时释放之。
    struct uart_reg volatile *my_reg;
    u32 baud;
    u32 rx_int_line,tx_int_line;    //发送和接收中断号
    ufast_t status;
    ufast_t serial_no;
};
//串口错误类型
enum uart_error_no
{
    enum_recv_fifo_over,         //接收硬件fifo溢出
    enum_recv_buf_over,          //接收缓冲区溢出
};

struct uart_reg
{
    u32 ULCON;     //0x50000000 0x50004000 0x50008000 Line control
    u32 UCON;      //0x50000004 0x50004004 0x50008004 Control
    u32 UFCON;     //0x50000008 0x50004008 0x50008008 FIFO control
    u32 UMCON;     //0x5000000c 0x5000400c 0x5000800c Modem control
    u32 UTRSTAT;   //0x50000010 0x50004010 0x50008010 Tx/Rx status
    u32 UERSTAT;   //0x50000014 0x50004014 0x50008014 Rx error status
    u32 UFSTAT;    //0x50000018 0x50004018 0x50008018 FIFO status
    u32 UMSTAT;    //0x5000001c 0x5000401c 0x5000801c Modem status

#ifdef __BIG_ENDIAN
    u8  reserve;
    u8  reserve1;
    u8  reserve2;
    u8  UTXH;      //0x50000023 0x50004023 0x50008023
    u8  reserve3;
    u8  reserve4;
    u8  reserve5;
    u8  URXH;      //0x50000027 0x50004027 0x50008027

#else //Little Endian
    u8  UTXH;      //0x50000020 0x50004020 0x50008020
    u8  reserve;   //UTXH和URXH是字节访问寄存器,reserve是占位变量.
    u8  reserve1;
    u8  reserve2;
    u8  URXH;      //0x50000024 0x50004024 0x50008024
    u8  reserve3;
    u8  reserve4;
    u8  reserve5;
 #endif
    u32 UBRDIV;    //0x50000028 0x50004028 0x50008028 Baud rate divisor
};

#define uart0_buf_len  512
#define uart1_buf_len  512
#define uart2_buf_len  512

//串口设备控制命令常数，用于uart_ctrl函数。
#define cn_uart_connect_recv_evtt     0  //设备被从右手写入的数据超过软件触发水
                                         //平时弹出本类型事件
#define cn_uart_disconnect_recv_evtt  1  //断开右手事件
#define cn_uart_connect_send_evtt     2  //设备被从左手写入数据时弹出本类型事件
#define cn_uart_disconnect_send_evtt  3  //断开左手事件
#define cn_uart_start                 4  //启动串口
#define cn_uart_stop                  5  //停止串口
#define cn_uart_set_baud              6  //设置uartBaud.
#define cn_uart_send_data             7  //发送数据
#define cn_uart_recv_data             8  //接收数据
#define cn_uart_completed_send        9  //完成发送工作
#define cn_uart_rx_pause              10 //暂停接收数据到缓冲区
#define cn_uart_rx_resume             11 //重新开始接收数据到缓冲区
#define cn_uart_overtime              12 //设置接收超时时间，毫秒数
#define cn_uart_recv_soft_level       13 //设置接收软件缓冲区触发水平
#define cn_uart_send_soft_level       14 //设置接收软件缓冲区触发水平
#define cn_uart_recv_hard_level       15 //设置接收fifo触发水平
#define cn_uart_send_hard_level       16 //设置发送fifo触发水平

ptu32_t uart_ctrl(struct  pan_device *uart_dev,u32 cmd,
                   ptu32_t data1,ptu32_t data2);
ptu32_t uart_app_write(struct  pan_device *uart_dev,ptu32_t src_buf,
                            ptu32_t res,ptu32_t len);
ptu32_t uart_hard_write(struct  pan_device *uart_dev,ptu32_t src_buf,
                            ptu32_t res,ptu32_t len);
ptu32_t uart_app_read(struct  pan_device *uart_dev,ptu32_t src_buf,
                            ptu32_t res,ptu32_t len);
ptu32_t uart_hard_read(struct  pan_device *uart_dev,ptu32_t src_buf,
                            ptu32_t res,ptu32_t len);
u32  uart0_int(ufast_t uart_int_line);
u32  uart1_int(ufast_t uart_int_line);
u32  uart2_int(ufast_t uart_int_line);
ptu32_t module_init_uart0(ptu32_t para);
ptu32_t module_init_uart1(ptu32_t para);
ptu32_t module_init_uart2(ptu32_t para);
void uart0_error_service(void);
void uart1_error_service(void);
void uart2_error_service(void);
void uart_send_service(void);

extern uint16_t u16g_evtt_uart0_error;
extern uint16_t u16g_evtt_uart1_error;

#ifdef __cplusplus
}
#endif

#endif // _UART_H_

