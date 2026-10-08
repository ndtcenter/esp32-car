#include <BluetoothSerial.h>


// ========== 引脚定义 ==========
#define IR_LED_PIN    19    // 红外发射控制引脚（高电平开启）
#define IR_ADC_PIN_1  32
#define IR_ADC_PIN_2  35
#define IR_ADC_PIN_3  34


// ========== 全局对象 ==========
BluetoothSerial SerialBT;

unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 100;  // 500ms

// ========== Teleplot 格式发送 ==========
void sendTeleplotData(float v1, float v2, float v3) {
    String output = "";
    output += ">ir1:" + String(v1, 2) + "\n";
    output += ">ir2:" + String(v2, 2) + "\n";
    output += ">ir3:" + String(v3, 2) + "\n";
    
    Serial.print(output);
    if (SerialBT.hasClient()) {
        SerialBT.print(output);
    }
}

// ========== 初始化 ==========
void setup() {
    Serial.begin(115200);
    SerialBT.begin("ESP32_IR_7788");

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);   // 量程 0~3.3V

    pinMode(IR_LED_PIN, OUTPUT);
    digitalWrite(IR_LED_PIN, LOW);    // 默认关闭

    Serial.println(">ESP32 IR Sensor Ready");
    Serial.println("Bluetooth Name: ESP32_IR_Sensor");
}

// ========== 主循环 ==========
void loop() {
    unsigned long now = millis();
    
    if (now - lastReadTime >= READ_INTERVAL) {
        lastReadTime = now;

        // ----- 1. 打开红外发射（高电平） -----
        digitalWrite(IR_LED_PIN, HIGH);
        
        // ----- 2. 等待 3ms 让红外稳定发射 -----
        delay(3);   // 3ms 延时，保证发射足够时间
        
        // ----- 3. 读取三路 ADC（此时红外正在发射） -----
        int adc1 = analogRead(IR_ADC_PIN_1);
        int adc2 = analogRead(IR_ADC_PIN_2);
        int adc3 = analogRead(IR_ADC_PIN_3);
        
        // ----- 4. 立即关闭红外发射 -----
        digitalWrite(IR_LED_PIN, LOW);

        // ----- 5. 转换并发送数据 -----
        float volt1 = adc1 * (3.3 / 4095.0);
        float volt2 = adc2 * (3.3 / 4095.0);
        float volt3 = adc3 * (3.3 / 4095.0);
        
        sendTeleplotData(volt1, volt2, volt3);
    }

    // （可选）蓝牙命令处理，这里保留但不影响主要功能
    if (SerialBT.available()) {
        char cmd = SerialBT.read();
        // 可添加自定义命令，例如通过蓝牙触发额外发射
    }
}