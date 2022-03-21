#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>

#define PIN_ENC_A   14
#define PIN_ENC_B   15
#define PIN_ENC_BTN 13

// Pin definitions and other options are inside platformio.ini
TFT_eSPI tft = TFT_eSPI();

RotaryEncoder *enc;
OneButton encBtn(PIN_ENC_BTN, false, false); // active high, no pullup (because shares LCD LED pin on cramped breadboard setup)
int32_t lastEncPos = 0;

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
}
