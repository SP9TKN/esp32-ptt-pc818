#pragma once

// Wyjście PTT — GPIO27 → R1 (330Ω) → LED w optoizolatorze PC817 → klucz PTT radia
#define PIN_PTT         27

// Przycisk ręcznego wyzwalania PTT (BOOT / S1), aktywny LOW, wciśnij i trzymaj = nadawanie
// (nadpisuje / uzupełnia sterowanie z CAT — przydatny do testu bez PC)
#define PIN_BTN_PTT     0

// Przycisk jasności podświetlenia — wbudowany S2 (GPIO35, tylko wejście, bez pull-up wewnętrznego)
#define PIN_BTN_BRIGHT  35

// Antydrgania przycisków (ms)
#define DEBOUNCE_MS     25

// Kolory ekranu (RGB565)
#define COLOR_TX        0xF800   // czerwony
#define COLOR_RX        0x07E0   // zielony
#define COLOR_TEXT      0xFFFF   // biały

// Rozmiar dużego napisu TX/RX (Font 4, skalowany)
#define BIG_TEXT_SIZE   4

// Ręczna korekta pozycji napisu względem środka (px) — patrząc od strony USB (po prawej)
#define TEXT_OFFSET_X   -4   // ujemne = w lewo
#define TEXT_OFFSET_Y    6   // dodatnie = w dół

// Podświetlenie — PWM (LEDC), te same wartości co w projekcie Bresser + 0 (wyłączony ekran)
#define BL_LEDC_FREQ    5000
#define BL_LEDC_BITS    8

// --- CAT (Icom CI-V) — emulacja PTT dla Icom IC-7100 ---
#define CIV_ADDR         0x88      // domyślny adres CI-V IC-7100
#define CIV_BAUD         9600      // baudrate CI-V do sterowania
#define FREQ_DEFAULT_HZ  14074000UL // raportowana częstotliwość (FT8, pasmo 20 m)
#define MODE_DEFAULT     0x01      // USB
#define FILTER_DEFAULT   0x01
