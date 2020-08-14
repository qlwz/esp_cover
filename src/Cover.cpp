#include "Cover.h"
#include "DOOYACommand.h"
#ifdef USE_HOMEKIT
#include "HomeKit.h"
#endif

#pragma region 继承

void Cover::init()
{
    if (config.weak_switch == 126)
    {
        config.weak_switch = 127;
    }
    if (config.power_switch == 126)
    {
        config.power_switch = 127;
    }
    if (config.direction == 126)
    {
        config.direction = 127;
    }
    if (config.hand_pull == 126)
    {
        config.hand_pull = 127;
    }
    softwareSerial = new SoftwareSerial(config.pin_rx, config.pin_tx); // RX, TX
    softwareSerial->begin(9600);
    if (config.pin_led != 99)
    {
        Led::init(config.pin_led > 30 ? config.pin_led - 30 : config.pin_led, config.pin_led > 30 ? HIGH : LOW);
    }
    if (config.pin_btn != 99)
    {
        pinMode(config.pin_btn, INPUT_PULLUP);
    }

    strcpy(positionStatTopic, Mqtt::getStatTopic(F("position")).c_str());
}

bool Cover::moduleLed()
{
    if (WiFi.status() == WL_CONNECTED && Mqtt::mqttClient.connected())
    {
        if (config.led == 0)
        {
            Led::on();
            return true;
        }
        else if (config.led == 1)
        {
            Led::off();
            return true;
        }
    }
    return false;
}

void Cover::loop()
{
    readSoftwareSerialTick();
    checkButton();

    if (bitRead(operationFlag, 0))
    {
        bitClear(operationFlag, 0);
        if (getPositionState || perSecond == 5 || perSecond % 60 == 0)
        {
            getPositionTask();
        }

        if (config.report_interval > 0 && (perSecond % config.report_interval) == 0)
        {
            reportPosition();
        }
    }
}

void Cover::perSecondDo()
{
    bitSet(operationFlag, 0);
}

void Cover::checkButton()
{
    bool buttonState = digitalRead(config.pin_btn);

    if (buttonState == 0)
    { // 按下按钮
        if (buttonTiming == false)
        {
            buttonTiming = true;
            buttonTimingStart = millis();
        }
        else
        { // buttonTiming = true
            if (millis() >= (buttonTimingStart + buttonDebounceTime))
            {
                buttonAction = 1;
            }

            if (millis() >= (buttonTimingStart + buttonLongPressTime))
            {
                buttonAction = 2;
            }
        }
    }
    else
    { // buttonState == 1, 释放按钮
        buttonTiming = false;

        if (buttonAction != 0)
        {
            if (buttonAction == 1) // 执行短按动作
            {
                Debug::AddInfo(PSTR("buttonShortPressAction"));
            }
            else if (buttonAction == 2) // 执行长按动作
            {
                Debug::AddInfo(PSTR("buttonLongPressAction"));
                Wifi::setupWifiManager(false);
            }
            buttonAction = 0;
        }
    }
}
#pragma endregion

#pragma region 配置

void Cover::readConfig()
{
    Config::moduleReadConfig(MODULE_CFG_VERSION, sizeof(CoverConfigMessage), CoverConfigMessage_fields, &config);
}

void Cover::resetConfig()
{
    Debug::AddInfo(PSTR("moduleResetConfig . . . OK"));
    memset(&config, 0, sizeof(CoverConfigMessage));
    config.position = 127;
    config.direction = 127;
    config.hand_pull = 127;
    config.weak_switch = 127;
    config.power_switch = 127;

    config.pin_rx = 14;
    config.pin_tx = 12;
    config.pin_led = 34;
    config.pin_btn = 13;

    config.autoStroke = 1;
}

void Cover::saveConfig(bool isEverySecond)
{
    Config::moduleSaveConfig(MODULE_CFG_VERSION, CoverConfigMessage_size, CoverConfigMessage_fields, &config);
}
#pragma endregion

#pragma region MQTT

void Cover::mqttCallback(char *topic, char *payload, char *cmnd)
{
    uint8_t tmp[10];
    uint8_t len = 0;
    if (strcmp(cmnd, "set_position") == 0)
    {
        uint8_t n = getInt(payload, 0, 100);
        len = DOOYACommand::setPosition(tmp, 0xFEFE, 0, n);
        getPositionState = true;
    }
    else if (strcmp(cmnd, "get_position") == 0)
    {
        len = DOOYACommand::getPosition(tmp, 0xFEFE, 0);
    }
    else if (strcmp(cmnd, "set") == 0)
    {
        if (strcmp(payload, "open") == 0)
        {
            len = DOOYACommand::open(tmp, 0xFEFE, 0);
            getPositionState = true;
        }
        else if (strcmp(payload, "close") == 0)
        {
            len = DOOYACommand::close(tmp, 0xFEFE, 0);
            getPositionState = true;
        }
        else if (strcmp(payload, "stop") == 0)
        {
            len = DOOYACommand::stop(tmp, 0xFEFE, 0);
            getPositionState = false;
        }
        else if (strcmp(payload, "position") == 0)
        {
            len = DOOYACommand::getPosition(tmp, 0xFEFE, 0);
        }
    }
    else if (strcmp(cmnd, "get") == 0)
    {
        if (strcmp(payload, "position") == 0)
        {
            len = DOOYACommand::getPosition(tmp, 0xFEFE, 0);
        }
        else if (strcmp(payload, "direction") == 0)
        {
            len = DOOYACommand::getDirectionStatus(tmp, 0xFEFE, 0);
        }
        else if (strcmp(payload, "hand_pull") == 0)
        {
            len = DOOYACommand::getHandPullStatus(tmp, 0xFEFE, 0);
        }
        else if (strcmp(payload, "motor") == 0)
        {
            len = DOOYACommand::getMotorStatus(tmp, 0xFEFE, 0);
        }
        else if (strcmp(payload, "weak_switch_type") == 0)
        {
            len = DOOYACommand::getWeakSwitchType(tmp, 0xFEFE, 0);
        }
        else if (strcmp(payload, "power_switch_type") == 0)
        {
            len = DOOYACommand::getPowerSwitchType(tmp, 0xFEFE, 0);
        }
        else if (strcmp(payload, "protocol_version") == 0)
        {
            len = DOOYACommand::getProtocolVersion(tmp, 0xFEFE, 0);
        }
    }
    else if (strcmp(cmnd, "delete_trip") == 0)
    {
        len = DOOYACommand::deleteTrip(tmp, 0xFEFE, 0);
    }
    else if (strcmp(cmnd, "reset") == 0)
    {
        len = DOOYACommand::reset(tmp, 0xFEFE, 0);
    }
    else if (strcmp(cmnd, "set_scene") == 0)
    {
        uint8_t n = getInt(payload, 1, 254);
        len = DOOYACommand::setScene(tmp, 0xFEFE, 0, n);
    }
    else if (strcmp(cmnd, "run_scene") == 0)
    {
        uint8_t n = getInt(payload, 1, 254);
        len = DOOYACommand::runScene(tmp, 0xFEFE, 0, n);
    }
    else if (strcmp(cmnd, "delete_scene") == 0)
    {
        uint8_t n = getInt(payload, 1, 254);
        len = DOOYACommand::deleteScene(tmp, 0xFEFE, 0, n);
    }
    else if (strcmp(cmnd, "open_close") == 0)
    {
        len = DOOYACommand::openOrClose(tmp, 0xFEFE, 0);
    }
    if (len > 0)
    {
        softwareSerial->write(tmp, len);
    }
}

void Cover::mqttDiscovery(bool isEnable)
{
    char topic[50];
    sprintf(topic, PSTR("%s/cover/%s/config"), globalConfig.mqtt.discovery_prefix, UID);
    if (isEnable)
    {
        char message[500];
        sprintf(message, PSTR("{\"name\":\"%s\","
                              "\"cmd_t\":\"%s\","
                              "\"pl_open\":\"open\","
                              "\"pl_cls\":\"close\","
                              "\"pl_stop\":\"stop\","
                              "\"pos_t\":\"%s\","
                              "\"set_pos_t\":\"%s\","
                              "\"position_open\":100,"
                              "\"position_closed\":0,"
                              "\"avty_t\":\"%s\","
                              "\"pl_avail\":\"online\","
                              "\"pl_not_avail\":\"offline\"}"),
                UID, Mqtt::getCmndTopic(F("set")).c_str(), positionStatTopic,
                Mqtt::getCmndTopic(F("set_position")).c_str(), Mqtt::getTeleTopic(F("availability")).c_str());
        //Debug::AddInfo(PSTR("discovery: %s - %s"), topic, message);
        Mqtt::publish(topic, message, true);
        Mqtt::availability();
    }
    else
    {
        Mqtt::publish(topic, "", true);
    }
}

void Cover::mqttConnected()
{
    if (globalConfig.mqtt.discovery)
    {
        mqttDiscovery(true);
    }
    getPositionTask();
}
#pragma endregion

#pragma region HTTP

void Cover::httpAdd(ESP8266WebServer *server)
{
    server->on(F("/cover_position"), std::bind(&Cover::httpPosition, this, server));
    server->on(F("/cover_set"), std::bind(&Cover::httpDo, this, server));
    server->on(F("/cover_setting"), std::bind(&Cover::httpSetting, this, server));
    server->on(F("/cover_reset"), std::bind(&Cover::httpReset, this, server));
    server->on(F("/ha"), std::bind(&Cover::httpHa, this, server));
#ifdef USE_HOMEKIT
    server->on(F("/homekit"), std::bind(&homekit_http, server));
#endif
}

String Cover::httpGetStatus(ESP8266WebServer *server)
{
    String data = F("\"position\":");
    data += config.position;
    return data;
}

void Cover::httpHtml(ESP8266WebServer *server)
{
    server->sendContent_P(
        PSTR("<table class='gridtable'><thead><tr><th colspan='2'>控制窗帘</th></tr></thead><tbody>"
             "<tr><td>操作</td><td><button type='button' class='btn-success' style='width:50px' id='cover_open' onclick=\"coverSet('open')\">开</button> "
             "<button type='button' class='btn-success' style='width:50px' onclick=\"coverSet('stop')\">停</button> "
             "<button type='button' class='btn-success' style='width:50px' id='cover_close' onclick=\"coverSet('close')\">关</button></td></tr>"));

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>当前位置</td><td><input type='range' min='0' max='100' id='position' name='position' value='%d' onchange='rangOnChange(this)'/>&nbsp;<span>%d%%</span></td></tr>"),
               config.position, config.position);
    server->sendContent_P(tmpData);

    server->sendContent_P(
        PSTR("<form method='post' action='/cover_setting' onsubmit='postform(this);return false'>"
             "<table class='gridtable'><thead><tr><th colspan='2'>窗帘设置</th></tr></thead><tbody>"
             "<tr><td>电机方向</td><td>"
             "<label class='bui-radios-label'><input type='radio' name='direction' value='0'/><i class='bui-radios'></i> 默认方向</label>&nbsp;&nbsp;&nbsp;&nbsp;"
             "<label class='bui-radios-label'><input type='radio' name='direction' value='1'/><i class='bui-radios'></i> 反方向</label>"
             "</td></tr>"

             "<tr><td>手拉使能</td><td>"
             "<label class='bui-radios-label'><input type='radio' name='hand_pull' value='0'/><i class='bui-radios'></i> 开启</label>&nbsp;&nbsp;&nbsp;&nbsp;"
             "<label class='bui-radios-label'><input type='radio' name='hand_pull' value='1'/><i class='bui-radios'></i> 关闭</label>"
             "</td></tr>"

             "<tr><td>弱点开关</td><td>"
             "<label class='bui-radios-label'><input type='radio' name='weak_switch' value='1'/><i class='bui-radios'></i> 双反弹开关</label><br/>"
             "<label class='bui-radios-label'><input type='radio' name='weak_switch' value='2'/><i class='bui-radios'></i> 双不反弹开关</label><br/>"
             "<label class='bui-radios-label'><input type='radio' name='weak_switch' value='3'/><i class='bui-radios'></i> DC246电子开关</label><br/>"
             "<label class='bui-radios-label'><input type='radio' name='weak_switch' value='4'/><i class='bui-radios'></i> 单键循环开关</label>"
             "</td></tr>"

             "<tr><td>强电开关</td><td>"
             "<label class='bui-radios-label'><input type='radio' name='power_switch' value='0'/><i class='bui-radios'></i> 强电双键不反弹模式</label><br/>"
             "<label class='bui-radios-label'><input type='radio' name='power_switch' value='1'/><i class='bui-radios'></i> 酒店模式</label><br/>"
             "<label class='bui-radios-label'><input type='radio' name='power_switch' value='2'/><i class='bui-radios'></i> 强电双键可反弹模式</label>"
             "</td></tr>"

             "<tr><td>LED</td><td>"
             "<label class='bui-radios-label'><input type='radio' name='led' value='0'/><i class='bui-radios'></i> 常亮</label>&nbsp;&nbsp;&nbsp;&nbsp;"
             "<label class='bui-radios-label'><input type='radio' name='led' value='1'/><i class='bui-radios'></i> 常灭</label>&nbsp;&nbsp;&nbsp;&nbsp;"
             "<label class='bui-radios-label'><input type='radio' name='led' value='2'/><i class='bui-radios'></i> 闪烁</label><br>未连接WIFI或者MQTT时为快闪"
             "</td></tr>"));

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>主动上报间隔</td><td><input type='number' min='0' max='3600' name='report_interval' required value='%d'>&nbsp;秒，0关闭</td></tr>"),
               config.report_interval);
    server->sendContent_P(tmpData);

    server->sendContent_P(
        PSTR("<tr><td>自动行程</td><td>"
             "<label class='bui-radios-label'><input type='radio' name='autoStroke' value='0'/><i class='bui-radios'></i> 否</label>&nbsp;&nbsp;&nbsp;&nbsp;"
             "<label class='bui-radios-label'><input type='radio' name='autoStroke' value='1'/><i class='bui-radios'></i> 是</label>"
             "</td></tr>"));

    server->sendContent_P(
        PSTR("<tr><td colspan='2'><button type='submit' class='btn-info'>设置</button><br>"
             "<button type='button' class='btn-success' style='margin-top:10px' onclick='window.location.href=\"/ha\"'>下载HA配置文件</button><br>"
             "<button type='button' class='btn-danger' style='margin-top: 10px' onclick=\"javascript:if(confirm('确定要电机恢复出厂设置？')){ajaxPost('/cover_reset');}\">电机恢复出厂</button>"
             "</td></tr>"
             "</tbody></table></form>"));

#ifdef USE_HOMEKIT
    homekit_html(server);
#endif
    server->sendContent_P(
        PSTR("<script type='text/javascript'>"
             "var iscover=0;function setDataSub(data,key){if(key=='position'){var t=id(key);var v=data[key];if(iscover>0&&v==t.value&&iscover++>5){iscover=0;intervalTime=defIntervalTime}t.value=v;t.nextSibling.nextSibling.innerHTML=v+'%';id('cover_open').disabled=v==100;id('cover_close').disabled=v==0;return true}return false}"
             "function coverSet(t){ajaxPost('/cover_set','do='+t);iscover=1;intervalTime=1000}function rangOnChange(the){the.nextSibling.nextSibling.innerHTML=the.value+'%';ajaxPost('/cover_position', 'position=' + the.value);iscover=1;intervalTime=1000}"));

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("setRadioValue('direction', '%d');"
                    "setRadioValue('hand_pull', '%d');"
                    "setRadioValue('weak_switch', '%d');"
                    "setRadioValue('power_switch', '%d');"
                    "setRadioValue('led', '%d');"
                    "setRadioValue('autoStroke', '%d');"),
               config.direction, config.hand_pull, config.weak_switch, config.power_switch, config.led, config.autoStroke);
    server->sendContent_P(tmpData);

    server->sendContent_P(PSTR("</script>"));
}

uint8_t Cover::getInt(char *str, uint8_t min, uint8_t max)
{
    uint8_t n = atoi(str);
    if (n > max)
        n = max;
    if (n < min)
        n = min;
    return n;
}

void Cover::httpPosition(ESP8266WebServer *server)
{
    int position = server->arg(F("position")).toInt();
    if (position > 100)
        position = 100;
    if (position < 0)
        position = 0;
    uint8_t tmp[10];
    int len = DOOYACommand::setPosition(tmp, 0xFEFE, 0, position);
    softwareSerial->write(tmp, len);
    delay(100);
    getPositionState = true;

    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->send_P(200, PSTR("application/json"), PSTR("{\"code\":1,\"msg\":\"已设置窗帘位置。\",\"data\":{"));
    server->sendContent(httpGetStatus(server));
    server->sendContent_P(PSTR("}}"));
}

void Cover::httpDo(ESP8266WebServer *server)
{
    String str = server->arg(F("do"));
    uint8_t tmp[10];
    int len;
    if (str.equals(F("open")))
    {
        len = DOOYACommand::open(tmp, 0xFEFE, 0);
        getPositionState = true;
    }
    else if (str.equals(F("close")))
    {
        len = DOOYACommand::close(tmp, 0xFEFE, 0);
        getPositionState = true;
    }
    else if (str.equals(F("stop")))
    {
        len = DOOYACommand::stop(tmp, 0xFEFE, 0);
        getPositionState = false;
    }
    else
    {
        server->send_P(200, PSTR("application/json"), PSTR("{\"code\":0,\"msg\":\"没有该操作。\"}"));
        return;
    }
    softwareSerial->write(tmp, len);
    delay(100);
    server->send_P(200, PSTR("application/json"), PSTR("{\"code\":1,\"msg\":\"已执行窗帘操作。\"}"));
}

void Cover::httpSetting(ESP8266WebServer *server)
{
    uint8_t tmp[10];
    int len;
    config.led = server->arg(F("led")).toInt();
    if (server->hasArg(F("report_interval")))
    {
        config.report_interval = server->arg(F("report_interval")).toInt();
    }
    if (server->hasArg(F("direction")))
    {
        uint8_t direction = server->arg(F("direction")).toInt();
        if (direction != config.direction)
        {
            len = DOOYACommand::setDirection(tmp, 0xFEFE, 0, direction != 1);
            softwareSerial->write(tmp, len);
            delay(100);

            len = DOOYACommand::getDirectionStatus(tmp, 0xFEFE, 0);
            softwareSerial->write(tmp, len);
            delay(100);

            len = DOOYACommand::getPosition(tmp, 0xFEFE, 0);
            softwareSerial->write(tmp, len);
            delay(100);
        }
    }

    if (server->hasArg(F("hand_pull")))
    {
        uint8_t handPull = server->arg(F("hand_pull")).toInt();
        if (handPull != config.hand_pull)
        {
            len = DOOYACommand::setHandPull(tmp, 0xFEFE, 0, handPull != 1);
            softwareSerial->write(tmp, len);
            delay(100);

            len = DOOYACommand::getHandPullStatus(tmp, 0xFEFE, 0);
            softwareSerial->write(tmp, len);
            delay(100);
        }
    }

    if (server->hasArg(F("weak_switch")))
    {
        config.weak_switch == 127;
        String weakSwitch = server->arg(F("weak_switch"));
        len = DOOYACommand::setWeakSwitchType(tmp, 0xFEFE, 0, weakSwitch.equals(F("2")) ? 0x02 : (weakSwitch.equals(F("3")) ? 0x03 : (weakSwitch.equals(F("4")) ? 0x04 : 0x01)));
        softwareSerial->write(tmp, len);
        delay(100);
    }

    if (server->hasArg(F("power_switch")))
    {
        config.power_switch == 127;
        String powerSwitch = server->arg(F("power_switch"));
        len = DOOYACommand::setPowerSwitchType(tmp, 0xFEFE, 0, powerSwitch.equals(F("1")) ? 0x01 : (powerSwitch.equals(F("2")) ? 0x02 : 0x00));
        softwareSerial->write(tmp, len);
    }

    server->send_P(200, PSTR("application/json"), PSTR("{\"code\":1,\"msg\":\"已经设置。\"}"));
}

void Cover::httpReset(ESP8266WebServer *server)
{
    server->send_P(200, PSTR("application/json"), PSTR("{\"code\":1,\"msg\":\"正在重置电机 . . . 设备将会重启。\"}"));
    delay(200);

    Led::blinkLED(400, 4);

    uint8_t tmp[10];
    int len;
    DOOYACommand::reset(tmp, 0xFEFE, 0);
    softwareSerial->write(tmp, len);
    delay(100);
    config.position = 127;
    config.direction = 127;
    config.hand_pull = 127;
    config.weak_switch = 127;
    config.power_switch = 127;
    Config::saveConfig();
    ESP.restart();
}

void Cover::httpHa(ESP8266WebServer *server)
{
    char attachment[100];
    snprintf_P(attachment, sizeof(attachment), PSTR("attachment; filename=%s.yaml"), UID);

    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->sendHeader(F("Content-Disposition"), attachment);
    server->send_P(200, PSTR("Content-Type: application/octet-stream"), "");

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("cover:\r\n"
                    "  - platform: mqtt\r\n"
                    "    name: \"%s\"\r\n"
                    "    command_topic: \"%s\"\r\n"
                    "    position_topic: \"%s\"\r\n"
                    "    set_position_topic: \"%s\"\r\n"
                    "    payload_open: \"open\"\r\n"
                    "    payload_close: \"close\"\r\n"
                    "    payload_stop: \"stop\"\r\n"
                    "    position_open: 100\r\n"
                    "    position_closed: 0\r\n"
                    "    availability_topic: \"%s\"\r\n"
                    "    payload_available: \"online\"\r\n"
                    "    payload_not_available: \"offline\"\r\n\r\n"),
               UID, Mqtt::getCmndTopic(F("set")).c_str(), positionStatTopic, Mqtt::getCmndTopic(F("set_position")).c_str(), Mqtt::getTeleTopic(F("availability")).c_str());
    server->sendContent_P(tmpData);
}
#pragma endregion

void Cover::doPosition(uint8_t position, uint8_t command)
{
    if (position == 255)
    {
        if (!config.autoStroke)
        {
            return;
        }
        // 自动设置行程
        Debug::AddInfo(PSTR("AutoStroke: %d"), autoStroke ? 1 : 0);
        if (!autoStroke)
        {
            autoStroke = true;
            delay(100);
            uint8_t tmp[10];
            uint8_t len2 = DOOYACommand::open(tmp, 0xFEFE, 0);
            softwareSerial->write(tmp, len2);
            getPositionState = true;
            if (config.position > 127)
            {
                config.position = 100;
            }
            Debug::AddInfo(PSTR("Start Auto Stroke: Last %d"), config.position);
        }
        return;
    }

    if (autoStroke && (position == 100 || position == 0)) // 设置完行程回到原来的位置
    {
        config.weak_switch = 127;
        config.power_switch = 127;
        autoStroke = false;
        delay(100);
        Debug::AddInfo(PSTR("Stop Auto Stroke: Last %d"), config.position);
        uint8_t tmp[10];
        uint8_t len2 = DOOYACommand::setPosition(tmp, 0xFEFE, 0, config.position);
        softwareSerial->write(tmp, len2);
        getPositionState = true;
    }
    else if (config.position != position)
    {
        Debug::AddInfo(PSTR("Position: %d"), position);
        config.position = position;
        Mqtt::publish(positionStatTopic, String(config.position).c_str(), globalConfig.mqtt.retain);
    }
}

void Cover::doSoftwareSerialTick(uint8_t *buf, int len)
{
    DOOYACommand::Command command = DOOYACommand::parserReplyCommand(buf, len);

    //char strHex[50];
    //DOOYACommand::hex2Str(buf, len, strHex, true);
    //Debug::AddInfo(PSTR("%s: %s"), (command.command == 0x01 ? "Read" : (command.command == 0x02 ? "Write" : (command.command == 0x03 ? "Control" : (command.command == 0x04 ? "Request" : "??")))), strHex);

    if (command.command == 0x01)
    {
        String topic;
        if (command.address == 0x02)
        {
            if (getPositionState && (command.data[0] == 0 || command.data[0] == 100))
            {
                getPositionState = false;
            }
            doPosition(command.data[0], command.command);
            return;
        }

        switch (command.address)
        {
        case 0x02:
            break;
        case 0x03:
            topic = Mqtt::getStatTopic(F("direction"));
            config.direction = command.data[0];
            break;
        case 0x04:
            topic = Mqtt::getStatTopic(F("hand_pull"));
            config.hand_pull = command.data[0];
            break;
        case 0x05:
            topic = Mqtt::getStatTopic(F("motor"));
            break;
        case 0x27:
            config.weak_switch = command.data[0];
            topic = Mqtt::getStatTopic(F("weak_switch_type"));
            break;
        case 0x28:
            config.power_switch = command.data[0];
            topic = Mqtt::getStatTopic(F("power_switch_type"));
            break;
        case 0xFE:
            topic = Mqtt::getStatTopic(F("protocol_version"));
            break;
        default:
            break;
        }
        if (topic.length() > 0)
        {
            Mqtt::publish(topic.c_str(), command.data, command.dataLen, globalConfig.mqtt.retain);
        }
    }
    else if (command.command == 0x02)
    {
    }
    else if (command.command == 0x03)
    {
    }
    else if (command.command == 0x04)
    {
        if (command.address == 0x02 && command.dataLen == 8)
        {
            // 0 : 百分比
            // 1 ：电机方向 0默认 1反向
            // 2 ：手拉使能 0开启 1关闭
            // 3 : 0电机停止 1开 2关 4电机卡停
            // 7 : 行程  0未设置行程  1已设置行程
            config.direction = command.data[1];
            config.hand_pull = command.data[2];
            if (command.data[3] == 0 || command.data[3] == 4) // 0电机停止 1 开 2 关 4 电机卡停
            {
                isRun = false;
                if (autoStroke && command.data[0] == 0xFF)
                {
                    uint8_t tmp[10];
                    uint8_t len2 = DOOYACommand::close(tmp, 0xFEFE, 0);
                    softwareSerial->write(tmp, len2);
                    getPositionState = true;
                    delay(100);
                }
                else
                {
                    getPositionState = false;
                }
            }
            else
            {
                isRun = true;
                getPositionState = true;
            }
            doPosition(command.data[0], command.command);
        }
    }
}

/*
  软串串口数据接受进程
 */
void Cover::readSoftwareSerialTick()
{
    while (softwareSerial->available())
    {
        uint8_t c = softwareSerial->read();
        Serial.printf("%02X ", c);
        if (softwareSerialPos == 0 && c != 0x55)
        { // 第一个字节不是0x55 抛弃
            Serial.print(F("\nBuff NO 0x55\n"));
            continue;
        }
        if (softwareSerialPos == 0)
        { // 记录0x55时间
            softwareSerialTime = millis();
        }
        softwareSerialBuff[softwareSerialPos++] = c;

        if (softwareSerialPos >= 6 && softwareSerialBuff[softwareSerialPos - 1] * 256 + softwareSerialBuff[softwareSerialPos - 2] == DOOYACommand::crc16(softwareSerialBuff, softwareSerialPos - 2)) // crc 正确
        {
            Serial.println();
            softwareSerialBuff[softwareSerialPos] = 0x00;
            doSoftwareSerialTick(softwareSerialBuff, softwareSerialPos);
            //Led::led(200);
            softwareSerialPos = 0;
        }
        else if (softwareSerialPos > 15)
        {
            Debug::AddInfo(PSTR("\nBuff Len Error"));
            softwareSerialPos = 0;
        }
    }

    if (softwareSerialPos > 0 && (millis() - softwareSerialTime >= 100))
    { // 距离0x55 超过100ms则抛弃指令
        softwareSerialPos = 0;
    }
}

void Cover::getPositionTask()
{
    uint8_t tmp[10];
    uint8_t len;
    if (config.weak_switch == 127)
    {
        config.weak_switch = 126;
        len = DOOYACommand::getWeakSwitchType(tmp, 0xFEFE, 0);
        softwareSerial->write(tmp, len);
        delay(100);
    }
    if (config.power_switch == 127)
    {
        config.power_switch = 126;
        len = DOOYACommand::getPowerSwitchType(tmp, 0xFEFE, 0);
        softwareSerial->write(tmp, len);
        delay(100);
    }
    if (config.hand_pull == 127)
    {
        config.hand_pull = 126;
        len = DOOYACommand::getHandPullStatus(tmp, 0xFEFE, 0);
        softwareSerial->write(tmp, len);
        delay(100);
    }
    if (config.direction == 127)
    {
        config.direction = 126;
        len = DOOYACommand::getDirectionStatus(tmp, 0xFEFE, 0);
        softwareSerial->write(tmp, len);
        delay(100);
    }

    len = DOOYACommand::getPosition(tmp, 0xFEFE, 0);
    softwareSerial->write(tmp, len);
}

void Cover::reportPosition()
{
    Mqtt::publish(positionStatTopic, String(config.position).c_str(), globalConfig.mqtt.retain);
}