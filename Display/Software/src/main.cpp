#include <TFT_eSPI.h>

// Pin definitions and other options are inside platformio.ini
TFT_eSPI tft = TFT_eSPI();

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
}

void loop() {
    tft.print(".");
    delay(500);
}
