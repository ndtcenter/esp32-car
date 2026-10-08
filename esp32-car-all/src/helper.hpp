#pragma once

#include <Arduino.h>

// ==========================================
// 1. PWM 直流电机驱动类 (重构后的 TaskMotor)
// ==========================================
class TaskMotor {
public:
    enum MotorState { FORWARD_STAGE, STOP1_STAGE, BACKWARD_STAGE, STOP2_STAGE };

    // 构造函数：接受两个引脚、电机名称、串口指针和状态切换间隔（默认1000ms）
    TaskMotor(int pinA, int pinB, const char* name, HardwareSerial *ser = nullptr, uint32_t intervalMs = 1000) {
        _pinA = pinA;
        _pinB = pinB;
        _name = name;
        _ser = ser;
        _interval = intervalMs;
        _state = STOP2_STAGE; // 初始状态
        
        // 自动分配独立的 ledc 通道（ESP32 有 16 个通道，0-15）
        // 这里的通道分配根据实例化的先后顺序静态递增
        _chanA = _nextChannel++;
        _chanB = _nextChannel++;
    }

    void begin() {
        // 配置 ESP32 的 ledc PWM 控制器
        // 频率选择 5kHz（直流电机常用频率），分辨率 8 位（0-255）
        ledcSetup(_chanA, 5000, 8);
        ledcSetup(_chanB, 5000, 8);

        // 将引脚绑定到对应的 PWM 通道
        ledcAttachPin(_pinA, _chanA);
        ledcAttachPin(_pinB, _chanB);

        // 初始让电机停止
        stop();
        _lastTime = millis();
    }

    // 基础驱动函数：正转，默认占空比 70% (在8位分辨率下，255 * 0.7 ≈ 178)
    void forward(int dutyPercent = 70) {
        int dutyValue = (dutyPercent * 255) / 100;
        ledcWrite(_chanA, dutyValue);  // A 桥输出 PWM
        ledcWrite(_chanB, 0);          // B 桥拉低
    }

    // 基础驱动函数：反转，默认占空比 70%
    void backward(int dutyPercent = 70) {
        int dutyValue = (dutyPercent * 255) / 100;
        ledcWrite(_chanA, 0);          // A 桥拉低
        ledcWrite(_chanB, dutyValue);  // B 桥输出 PWM
    }

    // 基础驱动函数：停止
    void stop() {
        ledcWrite(_chanA, 0);
        ledcWrite(_chanB, 0);
    }

    // 非阻塞轮询逻辑：循环运行“正转 -> 停转 -> 反转 -> 停转”
    void loop() {
        if (millis() - _lastTime >= _interval) {
            _lastTime = millis();
            
            // 状态机切换
            switch (_state) {
                case FORWARD_STAGE:  _state = STOP1_STAGE; break;
                case STOP1_STAGE:    _state = BACKWARD_STAGE; break;
                case BACKWARD_STAGE: _state = STOP2_STAGE; break;
                case STOP2_STAGE:    _state = FORWARD_STAGE; break;
            }
            
            // 根据当前状态执行对应的 PWM 操作（测试使用默认的 70% 占空比）
            updateMotorState();
            logState();
        }
    }

private:
    int _pinA, _pinB;
    uint8_t _chanA, _chanB;
    const char* _name;
    HardwareSerial *_ser;
    uint32_t _interval;
    uint32_t _lastTime = 0;
    MotorState _state;

    // 静态变量用于管理 ESP32 的全局 PWM 通道分配
    static uint8_t _nextChannel; 

    void updateMotorState() {
        switch (_state) {
            case FORWARD_STAGE:  forward(70);  break; // 正转 70%
            case STOP1_STAGE:    stop();       break; // 停止
            case BACKWARD_STAGE: backward(70); break; // 反转 70%
            case STOP2_STAGE:    stop();       break; // 停止
        }
    }

    void logState() {
        if (_ser) {
            _ser->print("[Motor "); _ser->print(_name); _ser->print("] State changed to: ");
            if (_state == FORWARD_STAGE) _ser->println("FORWARD (PWM 70%)");
            else if (_state == BACKWARD_STAGE) _ser->println("BACKWARD (PWM 70%)");
            else _ser->println("STOP");
        }
    }
};

// 初始化静态通道计数器（从通道 2 开始，避免与蜂鸣器/舵机冲突）
uint8_t TaskMotor::_nextChannel = 2;


// ==========================================
// 2. 其他外设类保持不变 (非阻塞)
// ==========================================
class TaskLED {
public:
    TaskLED(int pin, float frequencyHz, HardwareSerial *ser = nullptr) {
        _pin = pin; _ser = ser;
        _interval = (uint32_t)(1000.0 / (frequencyHz * 2.0));
    }
    void begin() { pinMode(_pin, OUTPUT); digitalWrite(_pin, _ledState); }
    void loop() {
        if (millis() - _lastTime >= _interval) {
            _lastTime = millis(); _ledState = !_ledState;
            digitalWrite(_pin, _ledState);
            if (_ser) { _ser->print("[LED Pin "); _ser->print(_pin); _ser->print("] "); _ser->println(_ledState ? "HIGH" : "LOW"); }
        }
    }
private:
    int _pin; HardwareSerial *_ser; uint32_t _interval; uint32_t _lastTime = 0; bool _ledState = false;
};

class TaskBuzzer {
public:
    TaskBuzzer(int pin, int freq = 2700, HardwareSerial *ser = nullptr) { _pin = pin; _freq = freq; _ser = ser; _ledcChannel = 0; }
    void begin() { ledcSetup(_ledcChannel, _freq, 8); ledcAttachPin(_pin, _ledcChannel); _lastTime = millis(); }
    void loop() {
        uint32_t currentInterval = _isTone ? _onTime : _offTime;
        if (millis() - _lastTime >= currentInterval) {
            _lastTime = millis(); _isTone = !_isTone;
            if (_isTone) { ledcWrite(_ledcChannel, 128); if (_ser) _ser->println("[Buzzer] ON"); } 
            else { ledcWrite(_ledcChannel, 0); if (_ser) _ser->println("[Buzzer] OFF"); }
        }
    }
private:
    int _pin; int _freq; HardwareSerial *_ser; uint8_t _ledcChannel; uint32_t _lastTime = 0; const uint32_t _onTime = 500; const uint32_t _offTime = 2000; bool _isTone = false;
};

class TaskServo {
public:
    /**
     * @param pin        舵机信号引脚
     * @param ser        调试串口（可选，nullptr 则无输出）
     * @param intervalMs 每步移动的间隔时间（毫秒）
     */
    TaskServo(int pin, HardwareSerial *ser = nullptr, uint32_t intervalMs = 500)
        : _pin(pin), _ser(ser), _interval(intervalMs) {
        _ledcChannel = 8;   // 自动分配空闲通道
    }

    ~TaskServo() {
        freeChannel(_ledcChannel);          // 析构时释放通道
        ledcDetachPin(_pin);
    }

    bool begin() {
        // 配置 LEDC: 50Hz (20ms 周期), 14位分辨率
        if (!ledcSetup(_ledcChannel, 50, 14)) {
            if (_ser) _ser->println("[Servo] ledcSetup failed!");
            return false;
        }
        ledcAttachPin(_pin, _ledcChannel);

        // 计算最小与最大占空比 (0.5ms 与 2.5ms 高电平)
        const uint32_t maxDuty = (1 << 14) - 1;  // 16383
        _minDuty = (uint32_t)(0.5f / 20.0f * maxDuty + 0.5f);
        _maxDuty = (uint32_t)(2.5f / 20.0f * maxDuty + 0.5f);

        // 边界保护
        if (_minDuty < 10) _minDuty = 10;
        if (_maxDuty > maxDuty - 10) _maxDuty = maxDuty - 10;

        // 生成平滑步长 (10步，从 _minDuty 到 _maxDuty 往返)
        for (int i = 0; i < 10; i++) {
            float t = (float)i / 9.0f;   // 0 ~ 1
            _steps[i] = (uint32_t)(_minDuty + t * (_maxDuty - _minDuty) + 0.5f);
        }

        // 初始位置：最小占空比
        _currentStep = 0;
        _stepDir = 1;
        ledcWrite(_ledcChannel, _steps[_currentStep]);
        _lastTime = millis();
        return true;
    }

    void loop() {
        if (millis() - _lastTime >= _interval) {
            _lastTime = millis();

            int nextStep = _currentStep + _stepDir;
            if (nextStep < 0) {
                nextStep = 1;
                _stepDir = 1;
            } else if (nextStep >= 10) {
                nextStep = 8;
                _stepDir = -1;
            }
            _currentStep = nextStep;

            ledcWrite(_ledcChannel, _steps[_currentStep]);

            if (_ser) {
                _ser->print("[Servo] Step: ");
                _ser->print(_currentStep);
                _ser->print("  Duty: ");
                _ser->println(_steps[_currentStep]);
            }
        }
    }

    // 手动设置位置 (步索引 0~9)
    void setStep(uint8_t step) {
        if (step >= 10) step = 9;
        _currentStep = step;
        ledcWrite(_ledcChannel, _steps[_currentStep]);
        _lastTime = millis();
    }

private:
    static const uint8_t MAX_LEDC_CHANNEL = 16;   // ESP32 LEDC 最大通道数
    static bool channelUsed[MAX_LEDC_CHANNEL];    // 通道占用标志

    static uint8_t allocateChannel() {
        for (uint8_t ch = 0; ch < MAX_LEDC_CHANNEL; ch++) {
            if (!channelUsed[ch]) {
                channelUsed[ch] = true;
                return ch;
            }
        }
        // 无可用通道，返回0（同时打印错误，由 begin 检查）
        return 0;
    }

    static void freeChannel(uint8_t channel) {
        if (channel < MAX_LEDC_CHANNEL) {
            channelUsed[channel] = false;
        }
    }

    int _pin;
    HardwareSerial *_ser;
    uint8_t _ledcChannel;
    uint32_t _lastTime = 0;
    uint32_t _interval;
    uint32_t _minDuty, _maxDuty;
    uint32_t _steps[10];
    int _currentStep;
    int _stepDir;
};

// 静态成员初始化
bool TaskServo::channelUsed[TaskServo::MAX_LEDC_CHANNEL] = {false};

class TaskHCSR04 {
public:
    TaskHCSR04(int trigPin, int echoPin, HardwareSerial *ser = nullptr) { _trigPin = trigPin; _echoPin = echoPin; _ser = ser; }
    void begin() { pinMode(_trigPin, OUTPUT); pinMode(_echoPin, INPUT); digitalWrite(_trigPin, LOW); _lastTime = millis(); }
    void loop() {
        if (millis() - _lastTime >= _measureInterval) {
            _lastTime = millis();
            digitalWrite(_trigPin, HIGH); delayMicroseconds(10); digitalWrite(_trigPin, LOW);
            long duration = pulseIn(_echoPin, HIGH, 20000); float distance = duration * 0.034 / 2;
            if (_ser) { _ser->print("[HC-SR04] Distance: "); if (duration == 0) _ser->println("Error"); else { _ser->print(distance); _ser->println(" cm"); } }
        }
    }
private:
    int _trigPin, _echoPin; HardwareSerial *_ser; uint32_t _lastTime = 0; const uint32_t _measureInterval = 200;
};