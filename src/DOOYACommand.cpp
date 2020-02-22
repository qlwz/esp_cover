#include <string.h>
#include "DOOYACommand.h"

const uint16_t crcTalbe[] = {
    0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
    0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400};

/**
 * 计算Crc16
 */
uint16_t DOOYACommand::crc16(uint8_t *ptr, uint16_t len)
{
    uint16_t crc = 0xffff;
    for (uint16_t i = 0; i < len; i++)
    {
        const uint8_t ch = *ptr++;
        crc = crcTalbe[(ch ^ crc) & 15] ^ (crc >> 4);
        crc = crcTalbe[((ch >> 4) ^ crc) & 15] ^ (crc >> 4);
    }
    return crc;
}

/**
 * Hex转Str
 */
int DOOYACommand::hex2Str(uint8_t *bin, uint16_t bin_size, char *buff, bool needBlank)
{
    const char *set = "0123456789ABCDEF";
    char *nptr = buff;
    if (NULL == buff)
    {
        return -1;
    }
    int len = needBlank ? (bin_size * 2 + bin_size) : (bin_size * 2 + 1);
    while (bin_size--)
    {
        *nptr++ = set[(*bin) >> 4];
        *nptr++ = set[(*bin++) & 0xF];
        if (needBlank && bin_size > 0)
        {
            *nptr++ = ' ';
        }
    }
    *nptr = '\0';
    return len;
}

/**
 * 生成命令
 */
uint16_t DOOYACommand::generateCommand(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t command, uint8_t address, uint8_t dataLen, uint8_t *data)
{
    int pos = 0;
    // 起始码
    outData[pos++] = 0x55;
    // 设备地址
    outData[pos++] = (0xff & id);
    outData[pos++] = ((0xff00 & id) >> 8);
    // 功能
    outData[pos++] = (channel << 4) | command;
    // 数据地址
    outData[pos++] = address;

    if (command == 0x01) // 读命令 （群控无效）
    {
        // 主机: 起始码 + 设备地址 + 0x01 + [数据地址 + 数据长度] + CRC16
        // 设备：起始码 + 设备地址 + 0x01 + [数据长度 + 数据内容] + CRC16
        outData[pos++] = dataLen;
    }
    else if (command == 0x02) // 写命令 （群控无效）
    {
        // 主机: 起始码 + 设备地址 + 0x02 + [数据地址 + 数据长度 + 数据内容] + CRC16
        // 设备：起始码 + 设备地址 + 0x02 + [数据地址 + 数据长度] + CRC16
        outData[pos++] = dataLen;
        memcpy(&outData[pos], data, dataLen);
        pos += dataLen;
    }
    else if (command == 0x03) // 控制命令
    {
        // 主机: 起始码 + 设备地址 + 0x03 + [指令 + 指令参数] + CRC16
        // 设备：起始码 + 设备地址 + 0x03 + [指令 + 指令参数] + CRC16[执行成功]
        // 设备：起始码 + 设备地址 + 0x03 + [指令 + 错误码] + CRC16[执行失败]

        if (dataLen > 0)
        {
            memcpy(&outData[pos], data, dataLen);
            pos += dataLen;
        }
    }
    else if (command == 0x04) // 请求命令 （由设备主动发起,请求主机处理）
    {
        // 设备：起始码 + 设备地址 + 0x04 + [请求参数] + CRC16
    }

    uint16_t crc = crc16(outData, pos);
    outData[pos++] = (0xff & crc);
    outData[pos++] = ((0xff00 & crc) >> 8);

    return pos;
}

/**
 * 生成读命令
 */
uint16_t DOOYACommand::generateReadCommand(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t dataAddress, uint8_t dataLen)
{
    return generateCommand(outData, id, channel, 0x01, dataAddress, dataLen, nullptr);
}

/**
 * 生成写命令
 */
uint16_t DOOYACommand::generateWriteCommand(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t dataAddress, uint8_t dataLen, uint8_t *data)
{
    return generateCommand(outData, id, channel, 0x02, dataAddress, dataLen, data);
}

/**
 * 生成控制命令
 */
uint16_t DOOYACommand::generateControlCommand(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t dataAddress, uint8_t dataLen, uint8_t *data)
{
    return generateCommand(outData, id, channel, 0x03, dataAddress, dataLen, data);
}

/**
 * 生成请求命令
 */
uint16_t DOOYACommand::generateRequestCommand(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t dataAddress, uint8_t dataLen, uint8_t *data)
{
    return generateCommand(outData, id, channel, 0x04, dataAddress, dataLen, data);
}

/**
 * 控制命令-打开
 */
uint16_t DOOYACommand::open(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateControlCommand(outData, id, channel, 0x01, 0, nullptr);
}

/**
 * 控制命令-关闭
 */
uint16_t DOOYACommand::close(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateControlCommand(outData, id, channel, 0x02, 0, nullptr);
}

/**
 * 控制命令-停止
 */
uint16_t DOOYACommand::stop(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateControlCommand(outData, id, channel, 0x03, 0, nullptr);
}

/**
 * 控制命令-设置百分比
 */
uint16_t DOOYACommand::setPosition(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t position)
{
    uint8_t data[] = {position};
    return generateControlCommand(outData, id, channel, 0x04, 1, data);
}

/**
 * 控制命令-删除行程
 */
uint16_t DOOYACommand::deleteTrip(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateControlCommand(outData, id, channel, 0x07, 0, nullptr);
}

/**
 * 控制命令-恢复出厂设置
 */
uint16_t DOOYACommand::reset(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateControlCommand(outData, id, channel, 0x08, 0, nullptr);
}

/**
 * 控制命令-设置情景模式
 */
uint16_t DOOYACommand::setScene(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t number)
{
    return generateControlCommand(outData, id, channel, 0x09, number, nullptr);
}

/**
 * 控制命令-运行情景模式
 */
uint16_t DOOYACommand::runScene(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t number)
{
    return generateControlCommand(outData, id, channel, 0x0A, number, nullptr);
}

/**
 * 控制命令-删除情景模式
 */
uint16_t DOOYACommand::deleteScene(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t number)
{
    return generateControlCommand(outData, id, channel, 0x0B, number, nullptr);
}

/**
 * 控制命令-取反
 */
uint16_t DOOYACommand::openOrClose(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateControlCommand(outData, id, channel, 0x0F, 0, nullptr);
}

/**
 * 读命令-位置（百分比）--0x02
 */
uint16_t DOOYACommand::getPosition(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateReadCommand(outData, id, channel, 0x02, 0x01);
}

/**
 * 读命令-方向状态 --0x03
 */
uint16_t DOOYACommand::getDirectionStatus(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateReadCommand(outData, id, channel, 0x03, 0x01);
}

/**
 * 读命令-手拉状态 --0x04
 */
uint16_t DOOYACommand::getHandPullStatus(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateReadCommand(outData, id, channel, 0x04, 0x01);
}

/**
 * 读命令-电机状态 --0x05
 */
uint16_t DOOYACommand::getMotorStatus(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateReadCommand(outData, id, channel, 0x05, 0x01);
}

/**
 * 读命令-弱电开关类型 --0x27
 */
uint16_t DOOYACommand::getWeakSwitchType(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateReadCommand(outData, id, channel, 0x27, 0x01);
}

/**
 * 读命令-强电开关类型 --0x28
 */
uint16_t DOOYACommand::getPowerSwitchType(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateReadCommand(outData, id, channel, 0x28, 0x01);
}

/**
 * 读命令-协议版本 --0xFE
 */
uint16_t DOOYACommand::getProtocolVersion(uint8_t *outData, uint16_t id, uint8_t channel)
{
    return generateReadCommand(outData, id, channel, 0xFE, 0x01);
}

/**
 * 写命令-写设备地址* --0x00
 */
uint16_t DOOYACommand::setProtocolVersion(uint8_t *outData, uint16_t id, uint8_t channel)
{
    uint8_t data[2];
    *(uint16_t *)(&data[0]) = id;
    return generateWriteCommand(outData, 0x0000, channel, 0x00, 0x02, data);
}

/**
 * 写命令-设置方向 --0x03
 */
uint16_t DOOYACommand::setDirection(uint8_t *outData, uint16_t id, uint8_t channel, bool isDefault)
{
    uint8_t data[] = {isDefault ? (uint8_t)0x00 : (uint8_t)0x01};
    return generateWriteCommand(outData, id, channel, 0x03, 0x01, data);
}

/**
 * 写命令-设置手拉使能 --0x04
 */
uint16_t DOOYACommand::setHandPull(uint8_t *outData, uint16_t id, uint8_t channel, bool isOpen)
{
    uint8_t data[] = {isOpen ? (uint8_t)0x00 : (uint8_t)0x01};
    return generateWriteCommand(outData, id, channel, 0x04, 0x01, data);
}

/**
 * 写命令-设置弱电开关类型 --0x27
 */
uint16_t DOOYACommand::setWeakSwitchType(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t type)
{
    uint8_t data[] = {type};
    return generateWriteCommand(outData, id, channel, 0x27, 0x01, data);
}

/**
 * 写命令-设置强电开关类型 --0x28
 */
uint16_t DOOYACommand::setPowerSwitchType(uint8_t *outData, uint16_t id, uint8_t channel, uint8_t type)
{
    uint8_t data[] = {type};
    return generateWriteCommand(outData, id, channel, 0x28, 0x01, data);
}

/**
 * 解析命令
 */
DOOYACommand::Command DOOYACommand::parserReplyCommand(uint8_t *recvBuf, int len)
{
    uint8_t pos = 0;

    Command command{};
    command.header = recvBuf[pos++];
    command.id = (recvBuf[pos++] << 8 | recvBuf[pos++]);
    uint8_t tmp = recvBuf[pos++];
    command.channel = tmp & 0xF0;
    command.command = tmp & 0x0F;
    if (command.command == 0x01)
    {
        command.address = recvBuf[pos++];
        command.dataLen = recvBuf[pos++];
        memcpy(command.data, &recvBuf[pos], command.dataLen);
        pos += command.dataLen;
    }
    else if (command.command == 0x02)
    {
        command.address = recvBuf[pos++];
        command.dataLen = recvBuf[pos++];
    }
    else if (command.command == 0x03)
    {
        command.address = recvBuf[pos++];
        command.dataLen = len - 7;
        if (command.dataLen > 0)
        {
            memcpy(command.data, &recvBuf[pos], command.dataLen);
            pos += command.dataLen;
        }
    }
    else if (command.command == 0x04)
    {
        command.address = recvBuf[pos++];
        if (len - pos > 2)
        {
            command.dataLen = recvBuf[pos++];
            if (len - pos - 2 == command.dataLen)
            {
                memcpy(command.data, &recvBuf[pos], command.dataLen);
                pos += command.dataLen;
            }
        }
    }

    return command;
}