#ifndef _DOOYACOMMAND_h
#define _DOOYACOMMAND_h

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

class DOOYACommand
{

public:
    struct Command
    {
        uint8_t header;
        uint16_t id;
        uint8_t channel;
        uint8_t command;
        uint8_t address;
        uint8_t dataLen;
        uint8_t data[20];
    };

    static uint16_t crc16(uint8_t *ptr, uint16_t len);
    static int hex2Str(uint8_t *bin, uint16_t bin_size, char *buff, bool needBlank);
    static uint16_t generateCommand(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t command, uint8_t address, uint8_t dataLen, uint8_t *data);
    static uint16_t generateReadCommand(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t dataAddress, uint8_t dataLen = 0x01);
    static uint16_t generateWriteCommand(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t dataAddress, uint8_t dataLen, uint8_t *data);
    static uint16_t generateControlCommand(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t dataAddress, uint8_t dataLen, uint8_t *data);
    static uint16_t generateRequestCommand(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t dataAddress, uint8_t dataLen, uint8_t *data);

    static uint16_t open(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t close(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t stop(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t setPosition(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t position);
    static uint16_t deleteTrip(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t reset(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t setScene(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t number);
    static uint16_t runScene(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t number);
    static uint16_t deleteScene(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t number);
    static uint16_t openOrClose(uint8_t *outData, uint16_t id, uint8_t channel);

    static uint16_t getPosition(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t getDirectionStatus(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t getHandPullStatus(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t getMotorStatus(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t getWeakSwitchType(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t getPowerSwitchType(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t getProtocolVersion(uint8_t *outData, uint16_t id, uint8_t channel);

    static uint16_t setProtocolVersion(uint8_t *outData, uint16_t id, uint8_t channel);
    static uint16_t setDirection(uint8_t *outData, uint16_t id, uint8_t channel, bool isDefault);
    static uint16_t setHandPull(uint8_t *outData, uint16_t id, uint8_t channel, bool isOpen);
    static uint16_t setWeakSwitchType(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t type);
    static uint16_t setPowerSwitchType(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t type);

    static Command parserReplyCommand(uint8_t *recvBuf, int len);
};

#endif