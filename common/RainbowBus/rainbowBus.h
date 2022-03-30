#pragma once

#include <stdint.h>
#include <string.h>

class RainbowBus {
    public:

    typedef struct {
        uint8_t dstId;      // 0 is master, 255 is broadcast
        uint8_t fromId;
        uint8_t packetType; // 0x00-0x0F is reserved for RainbowBus link layer
        uint16_t length;    // payload length
        // uint8_t payload[];
        // uint16_t checksum;   // checksum at end of packet
    } rbbHeader_t;

    enum class RbbPacketType {
        ACK = 0x01,
    };

    const uint8_t RbbBroadcastId = 255;


    RainbowBus(uint8_t id, void (*txFunc)(const uint8_t *buf, size_t size), void (*packetCallback)(rbbHeader_t *header, uint8_t *payload));
    void handleByte(uint8_t byte);
    void handleBytes(const uint8_t *bytes, size_t size);
    void sendPacket(uint8_t dstId, const uint8_t *payload, size_t payloadSize, bool needAck = false);
    // set txFunc
    // set callback
    // set id

    private:

    void (*_sendBytes)(const uint8_t *buf, size_t size);
    void (*_packetCallback)(rbbHeader_t *header, uint8_t *payload);
    uint8_t _id;
    uint8_t _awaitingAckFromId = 0xFF;
    uint8_t _curReadIdx;
    uint8_t *_packetBuf;
    uint16_t _curChecksum;
};