#include <ESP32Servo.h>

/**
 * 舵机控制类 (SG90)
 * - 20ms 时基 (50Hz)，高电平 0.5~2.5ms 对应 0°~180°
 * - 运动阶梯数可配置，默认 20 步
 * - 运动序列：先中点→最大角，然后最大↔最小无限往复
 */
class TaskServoMotor {
public:
    /**
     * 构造函数
     * @param pinA       舵机信号引脚 (必填)
     * @param pinB       备用引脚（本类未使用，保留占位）
     * @param ser        串口指针，用于打印调试信息 (默认 nullptr)
     * @param changeMs   单个方向总运动时间 (毫秒)，默认 60ms
     * @param totalStep  全程划分的阶梯数，默认 20
     */
    TaskServoMotor(int pinA, int pinB = -1, HardwareSerial *ser = nullptr,
               int changeMs = 60, int totalStep = 20)
        : _pin(pinA), _ser(ser), _changeMs(changeMs), _totalStep(totalStep),
          _currentAngle(90.0f) {}

    // 初始化舵机，并执行第一阶段：中间 → 最大角
    void begin() {
        _servo.attach(_pin, 500, 2500);   // 脉宽 0.5ms ~ 2.5ms
        _servo.setPeriodHertz(50);        // 50Hz → 20ms 周期
        delay(100);

        if (_ser) {
            _ser->println("ServoMotor ready on pin " + String(_pin));
            _ser->print("Total steps per direction: ");
            _ser->println(_totalStep);
            _ser->print("Movement time per direction: ");
            _ser->print(_changeMs);
            _ser->println(" ms");
        }

        // 先定位到中间 (90°)
        _servo.writeMicroseconds(1500);
        _currentAngle = 90.0f;
        delay(500);

        // 启动第一阶段：中 → 最大 (180°)
        startMove(180.0f);
        _phase = PHASE_INIT;
    }

    // 循环调用，驱动舵机执行阶梯运动
    void loop() {
        if (_stepsRemaining > 0) {
            // 执行一步
            _currentAngle += _stepDelta;
            if (--_stepsRemaining == 0) {
                _currentAngle = _targetAngle;   // 精确对准目标
                onMoveComplete();
            }
            int us = mapAngleToUs(_currentAngle);
            _servo.writeMicroseconds(us);

            // 每步延时 = 总时间 / 步数
            delay(_changeMs / _totalStep);
        } else {
            // 空闲时短暂等待，避免忙等
            delay(1);
        }
    }

private:
    int _pin;
    HardwareSerial *_ser;
    int _changeMs;
    int _totalStep;
    Servo _servo;

    float _currentAngle;    // 当前实际角度 (°)
    float _targetAngle;     // 目标角度
    int _stepsRemaining;    // 剩余步数
    float _stepDelta;       // 每步角度增量

    // 运动阶段
    enum Phase {
        PHASE_INIT,   // 初始阶段：中 → 最大 (仅一次)
        PHASE_LOOP    // 循环阶段：最大 ↔ 最小 (往复)
    } _phase;

    // 开始向目标角度运动 (自动分步)
    void startMove(float targetAngle) {
        _targetAngle = targetAngle;
        _stepsRemaining = _totalStep;
        _stepDelta = (targetAngle - _currentAngle) / (float)_totalStep;
    }

    // 单次运动完成时的回调
    void onMoveComplete() {
        if (_phase == PHASE_INIT) {
            // 初始阶段结束，切换至循环模式
            _phase = PHASE_LOOP;
            if (_ser) _ser->println("Init done. Entering loop: Max ↔ Min");
            // 开始第一次循环：最大 → 最小
            startMove(0.0f);
        } else {
            // 循环阶段：到达端点后反向
            float next = (_targetAngle == 0.0f) ? 180.0f : 0.0f;
            startMove(next);
            if (_ser) {
                _ser->print("Reverse to ");
                _ser->println(next);
            }
        }
    }

    // 角度 → 微秒 (线性映射: 0°→500us, 180°→2500us)
    int mapAngleToUs(float angle) {
        return (int)(500 + angle * (2000.0f / 180.0f));
    }
};

// ================= 全局对象 =================
// 使用 GPIO15 控制舵机，开启串口调试，总时间 60ms，20 阶梯
TaskServoMotor servo(15, -1, &Serial, 1200, 60);

void setup() {
    Serial.begin(115200);
    servo.begin();
}

void loop() {
    servo.loop();
}