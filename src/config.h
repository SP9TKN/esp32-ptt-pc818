#pragma once

// Wyjście PTT — GPIO27 → R (330Ω) → pin 1 (anoda) PC818, pin 2 (katoda) → GND
// Strona radia: pin 4 (kolektor) → PTT radia, pin 3 (emiter) → masa radia
#define PIN_PTT         27

// Diody LED — GPIO → R (330Ω) → anoda LED, katoda → GND
#define PIN_LED_RX      25   // zielona — nasłuch
#define PIN_LED_TX      26   // czerwona — nadawanie

// Przycisk ręcznego wyzwalania PTT (BOOT na płytce), aktywny LOW, wciśnij i trzymaj = nadawanie
// (uzupełnia sterowanie z CAT — przydatny do testu bez PC)
#define PIN_BTN_PTT     0

// Antydrgania przycisku (ms)
#define DEBOUNCE_MS     25

// --- CAT (Icom CI-V) — emulacja PTT dla Icom IC-7100 ---
#define CIV_ADDR         0x88      // domyślny adres CI-V IC-7100
#define CIV_BAUD         9600      // baudrate CI-V do sterowania
#define FREQ_DEFAULT_HZ  14074000UL // raportowana częstotliwość (FT8, pasmo 20 m)
#define MODE_DEFAULT     0x01      // USB
#define FILTER_DEFAULT   0x01
