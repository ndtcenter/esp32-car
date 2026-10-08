自研ESP32开发板，兼容 esp-wrover-kit；
使用的WIN11，使用了 CH340 用作USB转UART网桥实现下载功能，驱动版本 3.9.2024.9 ，日期2024/9/16，确认为最新驱动。
开发板和新旧笔记本均可正常通讯。
在自己的旧笔记本电脑上可以顺利的成功下载程序，在新笔记本电脑上总是下载失败，显示：

CURRENT: upload_protocol = esptool
Looking for upload port...
Auto-detected: COM5
Uploading .pio\build\esp-wrover-kit\firmware.bin
esptool.py v4.5.1
Serial port COM5
Connecting......................................

A fatal error occurred: Failed to connect to ESP32: Wrong boot mode detected (0x13)! The chip needs to be in download mode.
For troubleshooting steps visit: https://docs.espressif.com/projects/esptool/en/latest/troubleshooting.html
*** [upload] Error 2

