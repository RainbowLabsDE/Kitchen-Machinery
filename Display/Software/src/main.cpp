#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>
#include "api/one_wire.h"

#include "rainbowBus.h"


#define PIN_ENC_A       14
#define PIN_ENC_B       15
#define PIN_ENC_BTN     13
#define PIN_ONEWIRE     7
#define PIN_RS485_TX    4
#define PIN_RS485_RX    5
#define PIN_RS485_DE    6
#define RS485_BAUD      1000000

// Pin definitions and other options are inside platformio.ini
TFT_eSPI tft = TFT_eSPI();

RotaryEncoder *enc;
OneButton encBtn(PIN_ENC_BTN, false, false); // active high, no pullup (because shares LCD LED pin on cramped breadboard setup)
int32_t lastEncPos = 0;

One_wire oneWire(PIN_ONEWIRE);
rom_address_t tempAddr{};
uint32_t lastTempRequestMillis = 0;

UART rs485(PIN_RS485_TX, PIN_RS485_RX);

void rs485Write(const uint8_t *buf, size_t size) {
    digitalWrite(PIN_RS485_DE, HIGH);
    rs485.write(buf, size);
    rs485.flush();
    delayMicroseconds(15000000 / rs485.baud());   // delay needed, otherwise last byte is cut off and flush() doesn't do enough
    digitalWrite(PIN_RS485_DE, LOW);
}
RainbowBus rbb(rs485Write);


void encTick() {
    enc->tick();
}

void setup(void) {
    tft.init();
    tft.initDMA();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    // Set "cursor" at top left corner of display (0,0) and select font 4
    tft.setCursor(0, 0, 4);

    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Hello ");
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("World");
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.println("!");
    delay(1000);
    tft.setCursor(0, 32);
    tft.println(0);


    enc = new RotaryEncoder(PIN_ENC_A, PIN_ENC_B, RotaryEncoder::LatchMode::FOUR3);
    attachInterrupt(PIN_ENC_A, encTick, CHANGE);
    attachInterrupt(PIN_ENC_B, encTick, CHANGE);
    pinMode(PIN_ENC_A, INPUT_PULLUP);               // need to do pinMode after attachInterrupt, otherwise it get's overwritten (?)
    pinMode(PIN_ENC_B, INPUT_PULLUP);

    attachInterrupt(PIN_ENC_BTN, [] { encBtn.tick(); }, CHANGE);
    pinMode(PIN_ENC_BTN, INPUT_PULLDOWN);
    encBtn.attachClick([] { enc->setPosition(0); });


    oneWire.init();
    oneWire.single_device_read_rom(tempAddr);
    char buf[128];
    snprintf(buf, sizeof(buf), "%02x%02x%02x%02x%02x%02x%02x%02x\n", tempAddr.rom[0], tempAddr.rom[1], tempAddr.rom[2], tempAddr.rom[3], tempAddr.rom[4], tempAddr.rom[5], tempAddr.rom[6], tempAddr.rom[7]);
    oneWire.set_resolution(tempAddr, 12);
    tft.setCursor(0, 64);
    tft.print(buf);
    oneWire.convert_temperature(tempAddr, false, false);
    lastTempRequestMillis = millis();
    

    pinMode(PIN_RS485_DE, OUTPUT);
    rs485.begin(RS485_BAUD);
    // digitalWrite(PIN_RS485_DE, HIGH);
    // rs485.print("Hello World!!!!!!!!!!!!!!!!!!! 12345");
    // rs485.flush();
    // delayMicroseconds(15000000 / RS485_BAUD);   // delay needed, otherwise last byte is cut off and flush() doesn't do enough
    // digitalWrite(PIN_RS485_DE, LOW);
    char *str = "Hello World!!!!!!!!!!!!!!!!!!! 12345";
    // rs485Write((uint8_t*)str, strlen(str));
    rbb.sendPacket(1, (uint8_t*)str, strlen(str));

}


void loop() {
    int32_t encPos = enc->getPosition();
    if (encPos != lastEncPos) {
        lastEncPos = encPos;
        tft.setCursor(0, 32);
        tft.print("             ");
        tft.setCursor(0, 32);
        tft.println(encPos);
    }

    // also tick button in addition to interrupts
    encBtn.tick();


    if (millis() - lastTempRequestMillis > 750) {
        tft.setCursor(0, 96);
        float temp = oneWire.temperature(tempAddr);
        if (temp > -1000) {
            tft.print(temp);
            tft.println(" °C");
        }
        oneWire.convert_temperature(tempAddr, false, false);
        lastTempRequestMillis = millis();
    }

    int availableBytes = rs485.available();
    if (availableBytes) {
        // tft.print(rs485.read());
        uint8_t buf[availableBytes];
        rs485.readBytes(buf, availableBytes);
        rbb.handleBytes(buf, availableBytes);
    }
}
