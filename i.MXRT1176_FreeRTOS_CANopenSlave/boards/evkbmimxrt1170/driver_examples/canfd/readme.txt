板间CANFD通信测试程序
========

本例中，两块单板通过CAN总线连接。单板A向单板B发送CAN报文。
当单板B接收到CAN报文后回传给单板A，单板A接收到报文之后使用串口打印在串口调试助手上。


软件环境
===================
- 串口调试助手
- Keil MDK  5.38.1

硬件环境
=====================
- 串口线两根
- 测试板两块
- PC电脑一台
- CAN连接线一根


测试步骤
================
1. 正确连接串口线与CAN连接线.
2. 用以下配置打开串口调试助手:
   - 115200 baud rate
   - 8 data bits
   - No parity
   - One stop bit
   - No flow control
3. 烧录程序.
4. 复位.

运行现象
================
复位后，以下信息将会打印在串口调试助手中。
首先是选择测试板为A板或B板. (Note: Node B should start first)
然后数据会在A与B之间传输.
传输完成后须使用串口向A板发送数据开启下一次传输.

~~~~~~~~~~~~~~~~~~~~~
This message displays on the node A terminal:

********* FLEXCAN Interrupt EXAMPLE *********
    Message format: Standard (11 bit id)
    Message buffer 9 used for Rx.
    Message buffer 8 used for Tx.
    Interrupt Mode: Enabled
    Operation Mode: TX and RX --> Normal
*********************************************

Please select local node as A or B:
Note: Node B should start first.
Node:A
Press any key to trigger one-shot transmission

Rx MB ID: 0x123, Rx MB data: 0x0, Time stamp: 48411
Press any key to trigger the next transmission!

Rx MB ID: 0x123, Rx MB data: 0x1, Time stamp: 46583
Press any key to trigger the next transmission!
~~~~~~~~~~~~~~~~~~~~~

This message displays on the node B terminal:

********* FLEXCAN Interrupt EXAMPLE *********
    Message format: Standard (11 bit id)
    Message buffer 9 used for Rx.
    Message buffer 8 used for Tx.
    Interrupt Mode: Enabled
    Operation Mode: TX and RX --> Normal
*********************************************

Please select local node as A or B:
Note: Node B should start first.
Node:B
Start to Wait data from Node A

Rx MB ID: 0x321, Rx MB data: 0x0, Time stamp: 20174
Wait Node A to trigger the next transmission!

Rx MB ID: 0x321, Rx MB data: 0x1, Time stamp: 18385
Wait Node A to trigger the next transmission!
~~~~~~~~~~~~~~~~~~~~~
