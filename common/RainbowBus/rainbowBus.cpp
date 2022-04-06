#include "rainbowBus.h"
#include "crc.h"
#include <stdlib.h>
#include <stdio.h>

RainbowBus::RainbowBus(uint8_t id, void (*txFunc)(const uint8_t *buf, size_t size), void (*packetCallback)(rbbHeader_t *header, uint8_t *payload), uint32_t (*getCurrentMillis)(void)) {
    _id = id;
    _sendBytes = txFunc;
    _packetCallback = packetCallback;
    _getCurrentMillis = getCurrentMillis;
}

void RainbowBus::handleBytes(const uint8_t *bytes, size_t size) {
    // TODO: handle more elegantly
    for (size_t i = 0; i < size; i++) {
        handleByte(bytes[i]);
    }
    return;
}

void RainbowBus::handleByte(uint8_t byte) {
    // reset protocol handler after receiving an incomplete packet with timeout
    if (_curReadIdx > 0 && _getCurrentMillis() - _timeLastHandledByte > RbbTimeout) {
        _curReadIdx = 0;
    }
    
    printf("[RBB] %d %02X - ", _curReadIdx, byte);

    if (_curReadIdx == 0) {
        if (_packetBuf != NULL) {
            free(_packetBuf);
            _packetBuf = NULL;
        }
        _packetBuf = (uint8_t*)malloc(sizeof(rbbHeader_t));
        // TODO NULL handling
    }

    if (_packetBuf != NULL) {
        _packetBuf[_curReadIdx] = byte;
    }

    rbbHeader_t *header = (rbbHeader_t*)_packetBuf;
    if (_curReadIdx == sizeof(rbbHeader_t) - 1) {
        _packetBuf = (uint8_t*)realloc(_packetBuf, sizeof(rbbHeader_t) + header->length + sizeof(uint16_t));
        // TODO NULL handling
    }
    else if (_curReadIdx >= sizeof(rbbHeader_t) + header->length) {
        uint16_t checksumPos = _curReadIdx - sizeof(rbbHeader_t) - header->length;
        switch (checksumPos) {
            case 0:
                _curChecksum = byte;
                break;
            case 1:
                _curChecksum |= byte << 8;
                uint16_t calculatedCRC = crc16(_packetBuf, sizeof(rbbHeader_t) + header->length);
                if (_curChecksum == calculatedCRC) {
                    if (header->dstId == _id || header->dstId == RbbBroadcastId) {
                        // call callback
                        if (_packetBuf != NULL) {
                            _packetCallback((rbbHeader_t*)_packetBuf, _packetBuf + sizeof(rbbHeader_t));
                            free(_packetBuf);
                            _packetBuf = NULL;
                        }
                        else {
                            printf("This shouldn't have happened %s %d\n", __FILE__, __LINE__);
                        }
                    }
                }
                else {
                    printf("[RBB] Error: Checksums do not match!\n");
                }
                _curReadIdx = 0;
                return; // don't increment readIdx
                break;
        }
    }

    _curReadIdx++;
    _timeLastHandledByte = _getCurrentMillis();
    printf("\n");
}


void RainbowBus::sendPacket(uint8_t dstId, uint8_t packetType, const uint8_t *payload, size_t payloadSize, bool needAck) {
    uint8_t packetSize = sizeof(rbbHeader_t) + payloadSize + sizeof(uint16_t);
    uint8_t buf[packetSize];
    rbbHeader_t *header = (rbbHeader_t *)buf;

    header->dstId = dstId;
    header->fromId = _id;
    header->packetType = packetType;
    header->length = payloadSize;
    if (payload != NULL) {
        memcpy(buf + sizeof(rbbHeader_t), payload, payloadSize);                                                        // copy payload to packet buffer
    }
    uint16_t crc = crc16(buf, sizeof(rbbHeader_t) + payloadSize);     // calculate CRC and store at end of packet
    memcpy(buf + sizeof(rbbHeader_t) + payloadSize, &crc, sizeof(uint16_t));

    if (needAck) {
        _awaitingAckFromId = dstId;
    }
    _sendBytes(buf, packetSize);
}