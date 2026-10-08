# ESP32的引脚情况汇总
**自研**ESP32教学小车的引脚情况汇总：
1. IO27和IO14、IO25和IO26分别连接2个H桥的inA、inB输入,用于驱动2个DC Motor
2. IO2分别连接LED；
1. IO33连接2.7kHz的蜂鸣器，需要方波驱动；
1. IO12和G39连接HC-SR04；
1. IO15连接舵机SG90.
1. IO19连接了3路红外发射，IO32、35、34连接了3路ADC式的红外输入
1. IO13连接4颗WS2816C数字 RGB LED； 

采用Arduino框架，为上述各类外设设计非阻塞的单文件设备class，用于展示上述设备的应用
1. 电机 循环 正传、停转、反转、停转各1s；对应引脚直接digitalWrite 高低电平即可
2. LED 分别以0.5Hz、0.3Hz的频率闪烁；
3. 蜂鸣器 响半秒、停2秒；
4. 舵机从最大角每0.5s改变转到最小角度，分60个阶梯、每500ms变化;20ms时基，高电平范围 0.5~2.5ms;按如下模式运动
    1. 从中间转最大角；
    2. 从最大角转最小角；
    3. 从最小角转最大角；
    4. 往复循环2、3 
5. 通过串口输出所有状态改变；

参考：
`
class TaskMotor{
public:
    TaskMotor(int pinA,int pinB,HardwareSerial *ser=nullptr，int change=1000ms){...}
    void begin(){...}
    void loop(){...}
private:
    HardwareSerial *_ser=nullptr;
    ...

};

class TaskServo{
public:
    TaskServo(int pinSrvo,HardwareSerial *ser=nullptr，int change=500ms,int totalStep=60){...}
    void begin(){...}
    void loop(){...}
private:
    HardwareSerial *_ser=nullptr;
    ...

};
`


