#include <ESP8266WiFi.h>
#include <WiFiUdp.h>


const char* ssid = "SJ";
const char* password = "hcy08042";

WiFiUDP udp;
const unsigned int localPort = 8888;

void setup() {
  Serial.begin(115200);  // 与 Arduino UNO 通信的波特率
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  udp.begin(localPort);
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[32];
    int len = udp.read(buffer, sizeof(buffer)-1);
    if (len > 0) {
      buffer[len] = 0;
      // 将收到的数据通过串口发给 Arduino UNO
      Serial.println(buffer);
    }
  }
}