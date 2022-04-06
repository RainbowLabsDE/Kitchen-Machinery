#pragma once

#include <stdint.h>
#include <string.h>

#define RAINBOWBUS_BAUD 1000000

class RainbowBus {
    public:

    #pragma pack(push, 1)
    typedef struct {
        uint8_t dstId;      // 0 is master, 255 is broadcast
        uint8_t fromId;
        uint8_t packetType; // 0x00-0x0F is reserved for RainbowBus link layer
        uint16_t length;    // payload length
        // uint8_t payload[];
        // uint16_t checksum;   // checksum at end of packet
    } rbbHeader_t;
    #pragma pack(pop)

    enum class RbbPacketType {
        ACK = 0x01,
    };

    const static uint8_t RbbBroadcastId = 255;
    const static unsigned RbbTimeout = 5;       // ms


    RainbowBus(uint8_t id, void (*txFunc)(const uint8_t *buf, size_t size), void (*packetCallback)(rbbHeader_t *header, uint8_t *payload), uint32_t (*getCurrentMillis)(void));
    void handleByte(uint8_t byte);
    void handleBytes(const uint8_t *bytes, size_t size);
    void sendPacket(uint8_t dstId, uint8_t packetType, const uint8_t *payload = NULL, size_t payloadSize = 0, bool needAck = false);
    // set txFunc
    // set callback
    // set id

    private:

    void (*_sendBytes)(const uint8_t *buf, size_t size);
    void (*_packetCallback)(rbbHeader_t *header, uint8_t *payload);
    uint32_t (*_getCurrentMillis)();
    uint8_t _id;
    uint8_t _awaitingAckFromId = 0xFF;
    uint8_t _curReadIdx;
    uint8_t *_packetBuf;
    uint16_t _curChecksum;
    uint32_t _timeLastHandledByte;
};