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

## Bezpieczeństwo

Domyślnie skonfigurowane są trzy niezależne warstwy ochrony przed
podsłuchaniem strumienia audio przez osoby postronne:

1. **Hasło WiFi (WPA2)** — `WIFI_AP_PASSWORD` w `include/config.h`.
   **Zmień domyślne hasło przed użyciem.**
2. **Limit 1 połączonego urządzenia** (`WIFI_AP_MAX_CONN`) — dopóki Twój
   telefon jest podłączony do sieci ESP32, żadne inne urządzenie nie może
   do niej dołączyć, nawet znając hasło.
3. **Token aplikacyjny na WebSocket** (`WS_AUTH_TOKEN` w `config.h`,
   musi być identyczny z `WS_TOKEN` w `data/index.html`) — przeglądarka
   musi podać poprawny sekret zanim ESP zacznie wysyłać audio; złe/brak
   tokenu = natychmiastowe rozłączenie klienta.

**Koniecznie zmień oba domyślne sekrety** (`WIFI_AP_PASSWORD` i
`WS_AUTH_TOKEN`) na własne przed pierwszym użyciem — wartości w repo są
tylko przykładowe.

Ograniczenia, o których warto wiedzieć:
- Transmisja to zwykły `ws://` (bez TLS) — w obrębie własnej sieci WPA2 to
  akceptowalne ryzyko, ale to nie jest szyfrowanie end-to-end.
- Token w `index.html` jest widoczny w źródle strony dla każdego, kto ją
  wczyta — to obrona w głąb (utrudnienie), a nie zabezpieczenie kryptograficzne.
- Najsilniejszą warstwą jest w praktyce limit 1 klienta + hasło WiFi: nikt
  poza jednym połączonym urządzeniem nie ma w ogóle dostępu do sieci.

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
