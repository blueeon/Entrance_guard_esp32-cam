#include <Arduino.h>
#include <ArduinoJson.h>
#include <Tone32.h>
#include <common.h>

#define MODULE_NAME "esp32-cam"

// Wifi
#include <WiFi.h>
#define WIFI_SSID "Golink-iot"
#define WIFI_PASSWORD "golink0506"

// Mqtt服务
#include <PubSubClient.h>
// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topic = "blueeon_esp32/test";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

//二维码读头
#include <ESP32QRCodeReader.h>
ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);
struct QRCodeData qrCodeData;
bool isConnected = false;
WiFiClient espClient;
PubSubClient client(espClient);

// WIFI 状态灯
#define WIFI_LED_PIN 33
//通信波特率定义
#define MSG_Baud 115200
#define DEBUG_Baud 115200

bool connectWifi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return true;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int maxRetries = 10;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    maxRetries--;
    if (maxRetries <= 0)
    {
      return false;
    }
  }
  Serial.println();
  return true;
}
/**
 * @brief 对方收到消息时触发
 *
 * @param topic
 * @param payload
 * @param length
 */
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  String text = String((const char *)payload);
  notice("Topic(" + String(topic) + ") Message arrived：" + text);
}
/**
 * @brief 初始化Mqtt服务
 *
 */
void initMqttService()
{
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(mqttCallback);
  while (!client.connected())
  {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    notice("The mqtt client id: " + client_id);
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      notice("mqtt broker connected");
    }
    else
    {
      notice("failed with state " + client.state());
      delay(2000);
    }
  }
}

void setup()
{
  Serial.begin(DEBUG_Baud);
  //准备摄像头
  reader.setup();
  // reader.setDebug(true);
  notice("Setup QRCode Reader", MODULE_NAME);
  reader.begin();
  notice("Begin QR Code reader", MODULE_NAME);

  //初始化WiFi
  bool connected = connectWifi();
  if (connected)
  {
    isConnected = connected;
    digitalWrite(WIFI_LED_PIN, LOW);
    notice("Wifi is ready", MODULE_NAME);
  }
  else
  {
    notice("Wifi up failed", MODULE_NAME);
  }

  //对时
  configNTP();
  notice("NTP success", MODULE_NAME);
  //发送对时后的时间到串口
  DynamicJsonDocument doc(1024);
  doc["data"]["action"] = "ntp";
  doc["data"]["now"] = time(&now);
  sendMsgToSerial(doc, MODULE_NAME);
  //初始化Mqtt
  initMqttService();
  // publish and subscribe
  client.publish(topic, "Hi EMQX I'm ESP32 ^^");
  client.subscribe(topic);
  //初始化成功，发送模块准备好的消息给串口
  doc["data"]["action"] = "module_ready";
  doc["data"]["module_name"] = MODULE_NAME;
  sendMsgToSerial(doc, MODULE_NAME);
  delay(1000);
}
DynamicJsonDocument msgDoc(1024);
void loop()
{
  client.loop();
  if (reader.receiveQrCode(&qrCodeData, 200))
  {
    reader.end();
    notice("Found QRCode");
    String text = String((const char *)qrCodeData.payload);
    if (qrCodeData.valid)
    {

      notice("Payload: " + text);
      //扫码开门
      msgDoc["data"]["action"] = "open_door_by_qr";
      msgDoc["data"]["qrcode"] = text;
      //发送给串口
      sendMsgToSerial(msgDoc);
      // log
      client.publish(topic, (const char *)qrCodeData.payload);
    }
    else
    {
      notice("Invalid: " + text);
    }
    reader.begin();
  }

  delay(300);
}