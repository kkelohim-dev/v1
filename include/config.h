#pragma once

// ---- WiFi (tryb Access Point — ESP32 tworzy własną sieć) ----
#define WIFI_AP_SSID     "MeetingRecorder"
#define WIFI_AP_PASSWORD "nagrywanie123"   // min. 8 znakow, WPA2 — ZMIEN na wlasne!
#define WIFI_AP_CHANNEL  1
#define WIFI_AP_HIDDEN   0   // 1 = ukryte SSID (dodatkowa, slaba warstwa)
#define WIFI_AP_MAX_CONN 1   // tylko 1 urzadzenie moze byc podlaczone naraz

// Dodatkowy sekret wymagany przez przegladarke zanim ESP zacznie
// wysylac audio (obrona w glab, niezalezna od hasla WiFi).
// Musi byc identyczny z WS_TOKEN w data/index.html.
#define WS_AUTH_TOKEN "zmien-ten-tajny-kod"

// ---- Mikrofon I2S (INMP441) ----
#define I2S_MIC_PORT     I2S_NUM_0
#define I2S_WS_PIN       4   // LRCLK / WS
#define I2S_SCK_PIN      5   // BCLK / SCK
#define I2S_SD_PIN       6   // SD / DOUT mikrofonu -> wejscie ESP32

#define SAMPLE_RATE      16000
#define I2S_READ_SAMPLES 512        // probek na odczyt (~32ms przy 16kHz)

// Wzmocnienie: INMP441 zwraca probki 24-bit w slocie 32-bit.
// Przesuniecie w prawo skaluje sygnal do zakresu int16. Do regulacji po testach.
#define AUDIO_SHIFT_BITS 11

// ---- Karta SD (SPI) ----
#define SD_CS_PIN    10
#define SD_MOSI_PIN  11
#define SD_MISO_PIN  13
#define SD_SCK_PIN   12
