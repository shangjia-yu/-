/*
 * 无线体感机械臂 - 仅底座+大臂（双舵机版本）
 * 引脚分配：底座(9)，大臂/肩部(6)
 * 功能：接收 pitch（俯仰）控制大臂，roll（横滚）控制底座
 * 稳定性：中值滤波 + 速率限制 + 死区 + 低通滤波
 */

#include <SoftwareSerial.h>
#include <Servo.h>

// 软串口：RX=12 接 ESP8266 TX, TX=13 接 ESP8266 RX
SoftwareSerial espSerial(12, 13);

Servo servoBase;      // 底座舵机
Servo servoArm;       // 大臂（肩部）舵机

// ========== 可调参数（降低抖动） ==========
const float MAP_RANGE = 100.0;           // 映射范围（度），越大越不灵敏
const float DEAD_ZONE = 3.0;             // 死区（度），忽略微小变化
const float FILTER_ALPHA = 0.2;          // 低通滤波系数（越小越平滑）
const float MAX_RATE = 5.0;              // 舵机角度每帧最大变化量（度）
const int MEDIAN_SIZE = 5;               // 中值滤波窗口大小（奇数）

// 存储最近角度值用于中值滤波
float pitchBuf[MEDIAN_SIZE];
float rollBuf[MEDIAN_SIZE];
int bufIndex = 0;
bool bufReady = false;

// 上一次的舵机角度（用于速率限制）
float lastBase = 90, lastArm = 90;

// 中值滤波函数
float medianFilter(float *buffer, int size) {
  float sorted[size];
  memcpy(sorted, buffer, size * sizeof(float));
  for (int i = 0; i < size - 1; i++) {
    for (int j = i + 1; j < size; j++) {
      if (sorted[i] > sorted[j]) {
        float tmp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = tmp;
      }
    }
  }
  return sorted[size / 2];
}

void setup() {
  Serial.begin(9600);
  espSerial.begin(115200);

  servoBase.attach(9);   // 底座接引脚9
  servoArm.attach(6);    // 大臂接引脚6

  servoBase.write(90);
  servoArm.write(90);
  delay(1000);
}

void loop() {
  if (espSerial.available()) {
    String data = espSerial.readStringUntil('\n');
    int comma = data.indexOf(',');
    if (comma != -1) {
      float pitch = data.substring(0, comma).toFloat();  // 俯仰 -> 大臂
      float roll  = data.substring(comma + 1).toFloat(); // 横滚 -> 底座

      // 1. 中值滤波
      pitchBuf[bufIndex] = pitch;
      rollBuf[bufIndex] = roll;
      bufIndex = (bufIndex + 1) % MEDIAN_SIZE;
      if (bufIndex == 0) bufReady = true;

      float filteredPitch = pitch;
      float filteredRoll = roll;
      if (bufReady) {
        filteredPitch = medianFilter(pitchBuf, MEDIAN_SIZE);
        filteredRoll = medianFilter(rollBuf, MEDIAN_SIZE);
      }

      // 2. 死区处理
      static float lastPitch = 0, lastRoll = 0;
      if (abs(filteredPitch - lastPitch) < DEAD_ZONE && 
          abs(filteredRoll - lastRoll) < DEAD_ZONE) {
        return;
      }
      lastPitch = filteredPitch;
      lastRoll = filteredRoll;

      // 3. 映射到舵机角度（底座：0~180；大臂：30~150 防止碰撞）
      int rawBase = map(filteredRoll,  -MAP_RANGE, MAP_RANGE, 0, 180);
      int rawArm  = map(filteredPitch, -MAP_RANGE, MAP_RANGE, 30, 150);

      rawBase = constrain(rawBase, 0, 180);
      rawArm  = constrain(rawArm,  30, 150);

      // 4. 速率限制
      float deltaBase = rawBase - lastBase;
      float deltaArm  = rawArm - lastArm;
      deltaBase = constrain(deltaBase, -MAX_RATE, MAX_RATE);
      deltaArm  = constrain(deltaArm,  -MAX_RATE, MAX_RATE);

      float limitedBase = lastBase + deltaBase;
      float limitedArm  = lastArm + deltaArm;
      lastBase = limitedBase;
      lastArm  = limitedArm;

      // 5. 低通滤波
      static float smoothBase = 90, smoothArm = 90;
      smoothBase = FILTER_ALPHA * limitedBase + (1 - FILTER_ALPHA) * smoothBase;
      smoothArm  = FILTER_ALPHA * limitedArm  + (1 - FILTER_ALPHA) * smoothArm;

      // 6. 输出
      servoBase.write((int)smoothBase);
      servoArm.write((int)smoothArm);

      // 调试输出（可选，打开串口监视器 9600）
      Serial.print("p:"); Serial.print(filteredPitch);
      Serial.print(" r:"); Serial.print(filteredRoll);
      Serial.print(" base:"); Serial.print(smoothBase);
      Serial.print(" arm:"); Serial.println(smoothArm);
    }
  }
}