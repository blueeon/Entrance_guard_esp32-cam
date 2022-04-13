#include <Arduino.h>
#include <ArduinoJson.h>

#define MODULE_NAME "esp32-cam"

//时间戳
time_t now;
/**
 * @brief 一些debug信息
 *
 * @param notice
 * @return boolean
 */
boolean notice(String notice, const char *sensor = MODULE_NAME)
{
    DynamicJsonDocument doc(1024);
    doc["type"] = "notice";
    doc["sensor"] = sensor;
    doc["send_time"] = time(&now);
    doc["msg"] = notice;
    serializeJson(doc, Serial);
    Serial.println();
    return true;
}
boolean notice(const char *ch, const char *sensor = MODULE_NAME)
{
    return notice(String(ch), sensor);
}
/**
 * @brief 发送一个json消息给串口
 *
 * @param doc
 * @return boolean
 */
boolean sendMsgToSerial(DynamicJsonDocument doc, const char *sensor = MODULE_NAME)
{
    doc["type"] = "data";
    doc["sensor"] = sensor;
    doc["send_time"] = time(&now);
    serializeJson(doc, Serial);
    Serial.println();
    return true;
}

// NTP对时
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;

/**
 * @brief 对时间
 *
 */
void configNTP()
{
    //对时
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    while (time(&now) < 1000)
    {
        delay(500);
    }
    
}
