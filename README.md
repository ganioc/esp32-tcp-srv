# esp32-sensors Example

Starts a FreeRTOS task to print "esp32-sensors"

See the README.md file in the upper level 'examples' directory for more information about examples.


# Functions
## GPIO settings


## WIFI server

Ymodem protocol

SOH 0x01
seq 0x0000~0xFFFF
len 0xFFFF
Content
crc 16 校验 // 2字节

Content
cmd  // 1字节
target //

TX
01   // cmd, Setting
10   //  0x10(ESP32)
mode, // 01, 02, 03, 工作模式

RX
02  // cmd, Notification
obj // 01 (超声波), 02(开关1), 03(开关2), 0x10(ESP32)

