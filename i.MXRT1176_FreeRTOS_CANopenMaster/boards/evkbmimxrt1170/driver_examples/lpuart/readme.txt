串口测试程序

硬件环境
=====================
- UART1：	网口连接器（J5）
  UART2&5： 485 连接器（J7）
  UART3&8： 232 连接器（J6）
- 测试样板
- PC 电脑

测试步骤
================
1.  正确连接PC与开发板
2.  用以下配置打开串口:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3.  烧录程序.
4.  复位运行.

现象
================
当测试程序正确运行, 以下数据会打印在你的终端上:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Lpuart interrupt example
Board receives 8 characters then sends them out
Now please input:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

每当你使用PC发送4字节数据, 你发送的数据将会回传到PC.





