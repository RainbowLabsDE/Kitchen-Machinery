#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>
#include "api/one_wire.h"


#define PIN_ENC_A       14
#define PIN_ENC_B       15
#define PIN_ENC_BTN     13
#define PIN_ONEWIRE     7

// Pin definitions and other options are inside platformio.ini
TFT_eSPI tft = TFT_eSPI();

RotaryEncoder *enc;
OneButton encBtn(PIN_ENC_BTN, false, false); // active high, no pullup (because shares LCD LED pin on cramped breadboard setup)
int32_t lastEncPos = 0;

One_wire oneWire(PIN_ONEWIRE);
rom_address_t tempAddr{};
uint32_t lastTempRequestMillis = 0;


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
            tft.print(" °C");
        }
        oneWire.convert_temperature(tempAddr, false, false);
        lastTempRequestMillis = millis();
    }
}
