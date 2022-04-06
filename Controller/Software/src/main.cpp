#include <Arduino.h>
// #include <ETH.h>

#include "rainbowBus.h"
#include "slushLink.h"

#define PIN_ETH_TXEN    21
#define PIN_ETH_TXD0    19
#define PIN_ETH_TXD1    22
#define PIN_ETH_CRS_DV  27
#define PIN_ETH_RXD0    25
#define PIN_ETH_RXD1    26
#define PIN_ETH_MDC     23
#define PIN_ETH_MDIO    18
// clk?                 17
#define PIN_RS485_TX    5
#define PIN_RS485_RX    34
#define PIN_RS485_DE    33

#define PIN_SPI_CLK     14
#define PIN_SPI_MISO    12  // potentially remappable to GPI Pin (34-39)
#define PIN_SPI_MOSI    13
#define PIN_SPI_CS      15 

#define PIN_RPM_IN_1    35
#define PIN_RPM_IN_2    39

#define PIN_I2C_SCL     32
#define PIN_I2C_SDA     4

// pins 39 and 2 currently free, potentially 16 too


void rs485Write(const uint8_t *buf, size_t size) {
    digitalWrite(PIN_RS485_DE, HIGH);
    Serial1.write(buf, size);
    Serial1.flush();
    // delayMicroseconds(15000000 / rs485.baud());   // delay needed, otherwise last byte is cut off and flush() doesn't do enough
    digitalWrite(PIN_RS485_DE, LOW);
}
void incomingPacket(RainbowBus::rbbHeader_t *header, uint8_t *payload);
RainbowBus rbb(0, rs485Write, incomingPacket, (uint32_t (*)())millis);
bool rbbTransactionInProgress = false;

// TODO: place into own task
void incomingPacket(RainbowBus::rbbHeader_t *header, uint8_t *payload) {
    switch (header->packetType) {
        case SlushLink::SLPacketType::GetTemperature: {
            SlushLink::slPayload_GetTemperature_t *tp = (SlushLink::slPayload_GetTemperature_t *)payload;
            Serial.printf("Got Temp from ID %d: %6.2f°C\n", header->fromId, tp->temperature);
            break;
        }
        case SlushLink::SLPacketType::GetInterfaceStatus: {
            SlushLink::slPayload_GetInterfaceStatus_t *is = (SlushLink::slPayload_GetInterfaceStatus_t*)payload;
            Serial.printf("Got Interface from ID %d: enc: %3d, btn: %d\n", header->fromId, is->encoderDeltaPos, is->encoderBtnEvent );
            break;
        }
    }
    rbbTransactionInProgress = false;
}


void setup() {
    pinMode(2, OUTPUT); // LED and RS485 RE
    Serial.begin(115200);
    printf("KitchenMachinery Controller - Hello World!\n");

    pinMode(PIN_RS485_DE, OUTPUT);
    Serial1.begin(RAINBOWBUS_BAUD, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
    // pinMode(PIN_RS485_RX, INPUT_PULLUP);
    gpio_pullup_en((gpio_num_t)PIN_RS485_RX);
}

#define UX_INTERVAL 200
#define TEMP_INTERVAL 1500
uint32_t lastUxUpdate = 0, lastTempUpdate = 0;

void loop() {
    // digitalWrite(2, 1);
    // delay(250);
    // digitalWrite(2, 0);
    // delay(250);

    if (!rbbTransactionInProgress) {    // TODO: handle better
        if (millis() - lastUxUpdate >= UX_INTERVAL) {
            lastUxUpdate = millis();
            rbb.sendPacket(1, SlushLink::SLPacketType::GetInterfaceStatus);
            rbbTransactionInProgress = true;
        }
        else if (millis() - lastTempUpdate >= TEMP_INTERVAL) {
            lastTempUpdate = millis();
            rbb.sendPacket(1, SlushLink::SLPacketType::GetTemperature);
            rbbTransactionInProgress = true;
        }
    }

    int availableBytes = Serial1.available();
    if (availableBytes) {
        // tft.print(rs485.read());
        uint8_t buf[availableBytes];
        Serial1.readBytes(buf, availableBytes);
        rbb.handleBytes(buf, availableBytes);
    }
}