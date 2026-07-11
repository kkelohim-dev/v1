# Meeting Recorder — ESP32-S3

Firmware do ESP32-S3R8: nagrywa dźwięk z mikrofonu I2S, jednocześnie:
- transmituje audio na żywo przez WiFi do przeglądarki telefonu (WebSocket),
- zapisuje nagranie na kartę SD jako plik WAV.

## Sprzęt

- Płytka z chipem **ESP32-S3** (min. 8MB PSRAM, np. moduł R8)
- Mikrofon cyfrowy I2S **INMP441**
- Moduł karty **microSD** (SPI)

### Podłączenie mikrofonu INMP441

| INMP441 | ESP32-S3      |
|---------|---------------|
| VDD     | 3.3V          |
| GND     | GND           |
| WS      | GPIO4         |
| SCK     | GPIO5         |
| SD      | GPIO6         |
| L/R     | GND (kanał lewy) |

### Podłączenie karty SD (SPI)

| SD module | ESP32-S3 |
|-----------|----------|
| CS        | GPIO10   |
| MOSI      | GPIO11   |
| MISO      | GPIO13   |
| SCK       | GPIO12   |
| VCC       | 3.3V     |
| GND       | GND      |

Piny można zmienić w `include/config.h`.

## Wgrywanie firmware (PlatformIO)

```bash
# firmware
pio run --target upload

# strona www (data/index.html) na LittleFS
pio run --target uploadfs

# podgląd logów
pio device monitor
```

## Użycie

1. Po uruchomieniu ESP32 tworzy własną sieć WiFi:
   - SSID: `MeetingRecorder`
   - hasło: `nagrywanie123` (zmień w `config.h`)
2. Połącz telefon z tą siecią.
3. Otwórz w przeglądarce: `http://192.168.4.1/`
4. Kliknij **"Połącz i słuchaj"** — zaczniesz słyszeć dźwięk na żywo.
5. Kliknij **"Nagrywanie: Start"**, aby dodatkowo zapisać spotkanie na kartę SD
   (pliki `rec_000.wav`, `rec_001.wav`, ...).

## Uwagi / dostrajanie

- `AUDIO_SHIFT_BITS` w `config.h` reguluje wzmocnienie — jeśli dźwięk jest za
  cichy lub przesterowany, zmień tę wartość i wgraj ponownie.
- Domyślna częstotliwość próbkowania to 16 kHz (wystarczająca do mowy,
  oszczędza pasmo WiFi i miejsce na SD).
- Jeśli wolisz, żeby ESP32 łączył się z Twoją domową siecią WiFi zamiast
  tworzyć własną (tryb Access Point), zmień w `src/main.cpp`:
  `WiFi.mode(WIFI_AP); WiFi.softAP(...)` na `WiFi.begin(ssid, password)` —
  wtedy adres IP przydzieli router (sprawdzisz go w logu szeregowym).
- Jeśli karta SD nie zostanie wykryta przy starcie, live stream nadal
  działa — tylko zapis na SD jest wyłączony (patrz log w `pio device monitor`).
