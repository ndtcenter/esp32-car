#include "helper.hpp"


// ==========================================
// 全局外设对象实例化
// ==========================================

// 电机：内部自动分配不同的 PWM 通道。
// 电机A (IO27, IO14) 使用通道 2 和 3；电机B (IO25, IO26) 使用通道 4 和 5
TaskMotor motorA(27, 14, "A", &Serial, 1000);
TaskMotor motorB(25, 26, "B", &Serial, 1000);

// LED：IO19 (0.5Hz), IO2 (0.3Hz)
TaskLED led1(19, 0.5, &Serial);
TaskLED led2(2, 0.3, &Serial);

// 蜂鸣器：IO33 使用固定通道 0
TaskBuzzer buzzer(33, 2700, &Serial);

// 舵机：IO15 使用固定通道 1
TaskServo servo(15, &Serial);

// 超声波：Trig=IO12, Echo=G39
TaskHCSR04 hcsr04(12, 39, &Serial);


// ==========================================
// Arduino 入口
// ==========================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("--- ESP32 PWM Multi-tasking System Initialize ---");

    motorA.begin();
    motorB.begin();
    led1.begin();
    led2.begin();
    buzzer.begin();

    hcsr04.begin();

    servo.begin();
}

void loop() {
    motorA.loop();
    motorB.loop();
    led1.loop();
    led2.loop();
    // buzzer.loop();

    hcsr04.loop();

    // servo.loop();
}
