#include <ESP8266WiFi.h>
#include <Wire.h>
#include <WiFiUdp.h>

const char* ssid = "SJ";
const char* password = "hcy08042";

WiFiUDP udp;
const unsigned int localPort = 8888;
const char* broadcastAddress = "255.255.255.255";

const int MPU_addr = 0x68;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  // 唤醒MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  udp.begin(localPort);
}

void loop() {
  // 读取加速度数据
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 6, true);
  int16_t AcX = Wire.read()<<8|Wire.read();
  int16_t AcY = Wire.read()<<8|Wire.read();
  int16_t AcZ = Wire.read()<<8|Wire.read();

  // 计算俯仰(pitch)和横滚(roll)角度
  float pitch = atan2(AcY, AcZ) * 180 / PI;
  float roll  = atan2(AcX, AcZ) * 180 / PI;

  char buffer[32];
  sprintf(buffer, "%.1f,%.1f", pitch, roll);

  udp.beginPacket(broadcastAddress, localPort);
  udp.print(buffer);
  udp.endPacket();

  delay(50); // 每秒20次更新
}