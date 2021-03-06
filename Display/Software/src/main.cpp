#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>
#include "api/one_wire.h"

#include "rainbowBus.h"
#include "slushLink.h"

REDIRECT_STDOUT_TO(Serial); 


#define PIN_ENC_A       14
#define PIN_ENC_B       15
#define PIN_ENC_BTN     13
#define PIN_ONEWIRE     7
#define PIN_RS485_TX    4
#define PIN_RS485_RX    5
#define PIN_RS485_DE    6

// Pin definitions and other options are inside platformio.ini
TFT_eSPI tft = TFT_eSPI();

RotaryEncoder *enc;
OneButton encBtn(PIN_ENC_BTN, false, false); // active high, no pullup (because shares LCD LED pin on cramped breadboard setup)
int32_t lastEncPos = 0, lastEncoderPos = 0;
SlushLink::ButtonEvent lastBtnEvent = SlushLink::ButtonEvent::None;


One_wire oneWire(PIN_ONEWIRE);
rom_address_t tempAddr{};
uint32_t lastTempRequestMillis = 0;
float temperature = -1000;

// UART rs485(PIN_RS485_TX, PIN_RS485_RX);
#define RS485_UART  uart1

void rs485Write(const uint8_t *buf, size_t size) {
    digitalWrite(PIN_RS485_DE, HIGH);
    // rs485.write(buf, size);
    // rs485.flush();
    uart_write_blocking(RS485_UART, buf, size);
    delayMicroseconds(15000000 / RAINBOWBUS_BAUD);   // delay needed, otherwise last byte is cut off and flush() doesn't do enough
    digitalWrite(PIN_RS485_DE, LOW);
    // printf("> ");
    // for (int i = 0; i < size; i++) {
    //     printf("%02X ", buf[i]);
    // }
    // printf("\n");
}
void incomingPacket(RainbowBus::rbbHeader_t *header, uint8_t *payload);
RainbowBus rbb(1, rs485Write, incomingPacket, millis);


void encTick() {
    enc->tick();
}


void incomingPacket(RainbowBus::rbbHeader_t *header, uint8_t *payload) {
    printf("Incoming Packet from %d, to %d, type %02X, length %d\n", header->fromId, header->dstId, header->packetType, header->length);
    switch (header->packetType) {
        case SlushLink::SLPacketType::GetTemperature: {
            SlushLink::slPayload_GetTemperature_t payload = {
                .temperature = temperature
            };
            rbb.sendPacket(header->fromId, header->packetType, (uint8_t*)&payload, sizeof(payload));
            break;
        }
        case SlushLink::SLPacketType::GetInterfaceStatus: {
            SlushLink::slPayload_GetInterfaceStatus_t payload = {
                .encoderDeltaPos = (int8_t)(enc->getPosition() - lastEncoderPos),
                .encoderBtnEvent = lastBtnEvent
            };
            lastEncoderPos = enc->getPosition();
            lastBtnEvent = SlushLink::ButtonEvent::None;
            rbb.sendPacket(header->fromId, header->packetType, (uint8_t*)&payload, sizeof(payload));
            break;
        }
        case SlushLink::SLPacketType::SetDisplayContent: {
            SlushLink::slPayload_SetDisplayContent_t *dp = (SlushLink::slPayload_SetDisplayContent_t *)payload;
            tft.pushImageDMA(dp->x, dp->y, dp->w, dp->h, dp->data);
            // TODO: send ACK
            break;
        }
    }
}

RingBuffer rs485Buf;

void on_rs485_rx() {
    while (uart_is_readable(RS485_UART)) {
        uint8_t ch = uart_getc(RS485_UART);
        // printf("%02X \n", ch);
        if (rs485Buf.availableForStore()) {
            rs485Buf.store_char(ch);
        }
    }
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
    encBtn.attachClick([] { lastBtnEvent = SlushLink::ButtonEvent::Click; });
    encBtn.attachDoubleClick([] { lastBtnEvent = SlushLink::ButtonEvent::DoubleClick; });
    encBtn.attachMultiClick([] { lastBtnEvent = SlushLink::ButtonEvent::TripleClick; });
    encBtn.attachLongPressStart([] { lastBtnEvent = SlushLink::ButtonEvent::Hold; });
    encBtn.attachDuringLongPress([] { lastBtnEvent = SlushLink::ButtonEvent::Hold; });
    encBtn.attachLongPressStop([] { lastBtnEvent = SlushLink::ButtonEvent::Release; });


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
    // rs485.begin(RAINBOWBUS_BAUD);
    uint32_t actualBaud = uart_init(RS485_UART, RAINBOWBUS_BAUD);
    gpio_set_function(PIN_RS485_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_RS485_RX, GPIO_FUNC_UART);
    uart_set_fifo_enabled(RS485_UART, false);
    int UART_IRQ = RS485_UART == uart0 ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART_IRQ, on_rs485_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(RS485_UART, true, false);

    // digitalWrite(PIN_RS485_DE, HIGH);
    // rs485.print("Hello World!!!!!!!!!!!!!!!!!!! 12345");
    // rs485.flush();
    // delayMicroseconds(15000000 / RS485_BAUD);   // delay needed, otherwise last byte is cut off and flush() doesn't do enough
    // digitalWrite(PIN_RS485_DE, LOW);
    const char *str = "Hello World!!!!!!!!!!!!!!!!!!! 12345";
    // rs485Write((uint8_t*)str, strlen(str));
    // rbb.sendPacket(1, (uint8_t*)str, strlen(str));
    delay(3000);
    printf("KitchenMachinery Display - Hello World!\n");

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
        temperature = oneWire.temperature(tempAddr);
        if (temperature > -1000) {
            tft.print(temperature);
            tft.println(" ??C");
        }
        oneWire.convert_temperature(tempAddr, false, false);
        lastTempRequestMillis = millis();
    }

    // int availableBytes = rs485.available();
    // if (availableBytes) {
    //     // tft.print(rs485.read());
    //     uint8_t buf[availableBytes];
    //     rs485.readBytes(buf, availableBytes);
    //     rbb.handleBytes(buf, availableBytes);
    //     // printf("> ");
    //     // for (int i = 0; i < availableBytes; i++) {
    //     //     printf("%02X ", buf[i]);
    //     // }
    //     // printf("\n");
    // }

    if (rs485Buf.available()) {
        while (rs485Buf.available()) {
            // printf("%02X ", rs485Buf.read_char());
            rbb.handleByte(rs485Buf.read_char());
        }
        printf("\n");
    }
}
