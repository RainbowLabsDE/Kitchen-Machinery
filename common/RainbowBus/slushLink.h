#pragma once

#include <stdint.h>

class SlushLink {
    public:

    enum SLPacketType : uint8_t {
        // 0x00-0x0F is reserved for RainbowBus link layer
        GetTemperature = 0x10,
        GetInterfaceStatus,
        SetDisplayContent,
    };

    enum ButtonEvent : uint8_t {
        None,
        Click,
        DoubleClick,
        TripleClick,
        Hold,
        Release
    };

    #pragma pack(push, 1)
    typedef struct {
        float temperature;  // °C
    } slPayload_GetTemperature_t;

    typedef struct {
        int8_t encoderDeltaPos;
        ButtonEvent encoderBtnEvent;
    } slPayload_GetInterfaceStatus_t;

    typedef struct {
        int32_t x;
        int32_t y;
        int32_t w;
        int32_t h;
        uint16_t data[];
    } slPayload_SetDisplayContent_t;
    #pragma pack(pop)

    private:
};