#include "rainbowBus.h"
#include "crc.h"

RainbowBus::RainbowBus(void (*txFunc)(const uint8_t *buf, size_t size)) {
    _sendBytes = txFunc;
}

void RainbowBus::handleBytes(const uint8_t *bytes, size_t size) {
    // TODO
    return;
}

void RainbowBus::handleByte(uint8_t byte) {
    uint8_t bytes[] = { byte };
    return handleBytes(bytes, 1);
}


void RainbowBus::sendPacket(uint8_t dstId, const uint8_t *payload, size_t payloadSize, bool needAck) {
    uint8_t packetSize = rbbHeaderSize + payloadSize + sizeof(uint16_t);
    uint8_t buf[packetSize];
    rbbHeader_t *header = (rbbHeader_t *)buf;

    header->dstId = dstId;
    header->fromId = _id;
    header->length = payloadSize;
    memcpy(buf + rbbHeaderSize, payload, payloadSize);                                                  // copy payload to packet buffer
    update_crc16((uint16_t*)(buf + rbbHeaderSize + payloadSize), buf, rbbHeaderSize + payloadSize);     // calculate CRC and store at end of packet

    if (needAck) {
        _awaitingAckFromId = dstId;
    }
    _sendBytes(buf, packetSize);
}