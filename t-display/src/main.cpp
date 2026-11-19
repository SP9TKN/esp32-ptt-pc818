#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include "config.h"

TFT_eSPI tft = TFT_eSPI();
Preferences prefs;

enum PttState { STATE_RX, STATE_TX };
static PttState state = STATE_RX;

// --- przyciski (antydrgania) ---
struct Button {
    uint8_t pin;
    uint8_t mode = INPUT_PULLUP;   // GPIO34–39 nie mają pull-upu wewnętrznego → INPUT
    bool raw = HIGH, stable = HIGH, lastRaw = HIGH;
    unsigned long lastEdgeMs = 0;
    bool justPressed = false, justReleased = false;

    void begin() { pinMode(pin, mode); }

    void update() {
        justPressed = justReleased = false;
        raw = digitalRead(pin);
        unsigned long now = millis();
        if (raw != lastRaw) { lastEdgeMs = now; lastRaw = raw; }
        if ((now - lastEdgeMs) > DEBOUNCE_MS && raw != stable) {
            stable = raw;
            if (stable == LOW) justPressed = true; else justReleased = true;
        }
    }
};

static Button btnPtt    { PIN_BTN_PTT,    INPUT_PULLUP };
static Button btnBright { PIN_BTN_BRIGHT, INPUT };

// --- podświetlenie: te same poziomy PWM co w projekcie Bresser + 0 (wyłączony ekran) ---
static const uint8_t BRIGHT_LVL[] = {0, 10, 30, 70, 130, 190, 255};
static const uint8_t BRIGHT_STEPS = sizeof(BRIGHT_LVL) / sizeof(BRIGHT_LVL[0]);
static int8_t brightIdx = BRIGHT_STEPS - 1;   // start: pełna jasność (255)

void applyBrightness() {
    ledcWrite(TFT_BL, BRIGHT_LVL[brightIdx]);
    prefs.begin("disp", false);
    prefs.putUChar("bright", (uint8_t)brightIdx);
    prefs.end();
}

void dimScreen() {
    brightIdx--;
    if (brightIdx < 0) brightIdx = BRIGHT_STEPS - 1;   // po 0 (wyłączony) wraca do pełnej jasności
    applyBrightness();
}

// --- ekran ---
void drawState(PttState s) {
    uint16_t bg = (s == STATE_TX) ? COLOR_TX : COLOR_RX;
    const char *label = (s == STATE_TX) ? "TX" : "RX";

    tft.fillScreen(bg);
    tft.setTextColor(COLOR_TEXT, bg);
    tft.setTextFont(4);
    tft.setTextSize(BIG_TEXT_SIZE);

    // ręczne wyliczenie środka (niezależne od datum) — łatwe do zweryfikowania
    int16_t tw = tft.textWidth(label);
    int16_t th = tft.fontHeight();
    int16_t x  = (tft.width()  - tw) / 2 + TEXT_OFFSET_X;
    int16_t y  = (tft.height() - th) / 2 + TEXT_OFFSET_Y;

    tft.setTextDatum(TL_DATUM);
    tft.drawString(label, x, y);
}

void setState(PttState s) {
    if (s == state) return;
    state = s;
    digitalWrite(PIN_PTT, s == STATE_TX ? HIGH : LOW);
    drawState(s);
}

// --- CAT: Icom CI-V (emulacja IC-7100, tylko zakres potrzebny do PTT) ---
static uint8_t civBuf[16];
static uint8_t civLen = 0;
static bool civCapturing = false;
static uint8_t civPrevByte = 0;

void civSend(uint8_t toAddr, const uint8_t *payload, uint8_t len) {
    Serial.write(0xFE);
    Serial.write(0xFE);
    Serial.write(toAddr);
    Serial.write(CIV_ADDR);
    Serial.write(payload, len);
    Serial.write(0xFD);
}

void civEncodeFreq(uint32_t hz, uint8_t *out5) {
    for (int i = 0; i < 5; i++) {
        uint32_t v = hz % 100;
        out5[i] = ((v / 10) << 4) | (v % 10);
        hz /= 100;
    }
}

void civHandleFrame(uint8_t *buf, uint8_t len) {
    if (len < 3) return;
    uint8_t toAddr   = buf[0];
    uint8_t fromAddr = buf[1];
    uint8_t cmd      = buf[2];

    if (toAddr != CIV_ADDR && toAddr != 0x00) return;   // ramka nie do nas

    if (cmd == 0x03 && len == 3) {                       // odczyt częstotliwości
        uint8_t d[6] = {0x03};
        civEncodeFreq(FREQ_DEFAULT_HZ, &d[1]);
        civSend(fromAddr, d, 6);
        return;
    }

    if (cmd == 0x04 && len == 3) {                        // odczyt trybu
        uint8_t d[3] = {0x04, MODE_DEFAULT, FILTER_DEFAULT};
        civSend(fromAddr, d, 3);
        return;
    }

    if (cmd == 0x1C && len >= 4 && buf[3] == 0x00) {       // PTT (Transceive status)
        if (len >= 5) {
            bool tx = (buf[4] == 0x01);
            setState(tx ? STATE_TX : STATE_RX);
            uint8_t ok[1] = {0xFB};                        // potwierdzenie OK
            civSend(fromAddr, ok, 1);
        } else {
            uint8_t d[3] = {0x1C, 0x00, (uint8_t)(state == STATE_TX ? 0x01 : 0x00)};
            civSend(fromAddr, d, 3);
        }
        return;
    }
}

void civFeed(uint8_t b) {
    if (!civCapturing) {
        if (civPrevByte == 0xFE && b == 0xFE) {
            civCapturing = true;
            civLen = 0;
        }
        civPrevByte = b;
        return;
    }

    if (b == 0xFE) { civLen = 0; return; }                // nowy początek ramki

    if (b == 0xFD) {
        civHandleFrame(civBuf, civLen);
        civCapturing = false;
        civLen = 0;
        civPrevByte = 0;
        return;
    }

    if (civLen < sizeof(civBuf)) civBuf[civLen++] = b;
    else civCapturing = false;                            // przepełnienie — porzuć ramkę
}

void setup() {
    pinMode(PIN_PTT, OUTPUT);
    digitalWrite(PIN_PTT, LOW);

    btnPtt.begin();
    btnBright.begin();

    tft.init();
    tft.setRotation(1);       // poziomo, 240×135

    prefs.begin("disp", true);
    brightIdx = (int8_t)prefs.getUChar("bright", BRIGHT_STEPS - 1);
    prefs.end();

    ledcAttach(TFT_BL, BL_LEDC_FREQ, BL_LEDC_BITS);
    ledcWrite(TFT_BL, BRIGHT_LVL[brightIdx]);   // przywrócona jasność — bez zapisu do NVS

    Serial.begin(CIV_BAUD);

    drawState(STATE_RX);
}

void loop() {
    while (Serial.available()) civFeed(Serial.read());

    btnPtt.update();
    if (btnPtt.justPressed)  setState(STATE_TX);
    if (btnPtt.justReleased) setState(STATE_RX);

    btnBright.update();
    if (btnBright.justPressed) dimScreen();
}
