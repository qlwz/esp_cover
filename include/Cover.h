// Cover.h
#ifndef _COVER_h
#define _COVER_h

#include <SoftwareSerial.h>
#include "Module.h"
#include "CoverConfig.pb.h"

#define MODULE_CFG_VERSION 1502 //1501 - 2000

const char HASS_DISCOVER_COVER[] PROGMEM =
    "{\"name\":\"%s\","
    "\"cmd_t\":\"%s\","
    "\"pl_open\":\"OPEN\","
    "\"pl_cls\":\"CLOSE\","
    "\"pl_stop\":\"STOP\","
    "\"pos_t\":\"%s\","
    "\"set_pos_t\":\"%s\","
    "\"position_open\":100,"
    "\"position_closed\":0,"
    "\"avty_t\":\"%s\","
    "\"pl_avail\":\"online\","
    "\"pl_not_avail\":\"offline\"}";

class Cover : public Module
{
private:
    uint8_t operationFlag = 0; // 0每秒

    CoverConfigMessage config;

    boolean getPositionState = false;
    SoftwareSerial *softwareSerial;       // RX, TX
    uint8_t softwareSerialBuff[20];       // 定义缓冲buff
    uint8_t softwareSerialPos = 0;        // 收到的字节实际长度
    unsigned long softwareSerialTime = 0; // 记录读取最后一个字节的时间点
    boolean autoStroke = false;           // 是否自动设置行程

    // 按键
    int buttonDebounceTime = 50;
    int buttonLongPressTime = 2000; // 2000 = 2s
    boolean buttonTiming = false;
    unsigned long buttonTimingStart = 0;
    int buttonAction = 0; // 0 = 没有要执行的动作, 1 = 执行短按动作, 2 = 执行长按动作

    uint8_t getInt(String str, uint8_t min, uint8_t max);
    void httpPosition(ESP8266WebServer *server);
    void httpDo(ESP8266WebServer *server);
    void httpSetting(ESP8266WebServer *server);
    void httpReset(ESP8266WebServer *server);
    void httpHa(ESP8266WebServer *server);

    void doPosition(uint8_t position, uint8_t command);
    void doSoftwareSerialTick(uint8_t *buf, int len);

    void readSoftwareSerialTick();
    void getPositionTask();
    void checkButton();

    void reportPosition();

public:
    void init();
    String getModuleName() { return F("cover"); }
    String getModuleCNName() { return F("杜亚KT82TV/W窗帘电机"); }
    String getModuleVersion() { return F("2020.02.22.2100"); }
    String getModuleAuthor() { return F("情留メ蚊子"); }
    bool moduleLed();

    void loop();
    void perSecondDo();

    void readConfig();
    void resetConfig();
    void saveConfig(bool isEverySecond);

    void mqttCallback(String topicStr, String str);
    void mqttConnected();
    void mqttDiscovery(boolean isEnable = true);

    void httpAdd(ESP8266WebServer *server);
    void httpHtml(ESP8266WebServer *server);
    String httpGetStatus(ESP8266WebServer *server);
};
#endif