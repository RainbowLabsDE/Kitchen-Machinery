#include <TFT_eSPI.h>
#include <RotaryEncoder.h>

#define PIN_ENC_A   14
#define PIN_ENC_B   15
#define PIN_ENC_BTN 13

// Pin definitions and other options are inside platformio.ini
TFT_eSPI tft = TFT_eSPI();

RotaryEncoder *enc;
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


    enc = new RotaryEncoder(PIN_ENC_A, PIN_ENC_B, RotaryEncoder::LatchMode::FOUR0);
    pinMode(PIN_ENC_A, INPUT_PULLDOWN);
    pinMode(PIN_ENC_B, INPUT_PULLDOWN);
    attachInterrupt(PIN_ENC_A, encTick, CHANGE);
    attachInterrupt(PIN_ENC_B, encTick, CHANGE);
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
}
