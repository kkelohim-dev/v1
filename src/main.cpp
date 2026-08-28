// Meeting Recorder — ESP32-S3
// Mikrofon I2S -> (1) live stream WebSocket do przeglądarki telefonu
//              -> (2) zapis WAV na kartę SD
//
// Sieć: ESP32 tworzy własny WiFi AP. Telefon łączy się i otwiera
// http://192.168.4.1/ w przeglądarce.

#include <Arduino.h>
#include <WiFi.h>
#include <driver/i2s.h>
#include <SPI.h>
#include <SD.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <set>

#include "config.h"
#include "wav_writer.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Klienci, ktorzy podali poprawny WS_AUTH_TOKEN — tylko oni dostają
// audio i moga sterowac nagrywaniem.
static std::set<uint32_t> authorizedClients;

static int32_t  i2sRawBuf[I2S_READ_SAMPLES];
static int16_t  pcmBuf[I2S_READ_SAMPLES];

static bool sdReady = false;
static bool recording = false;
static File recordFile;
static uint32_t dataBytesWritten = 0;
static uint32_t recordingIndex = 0;

static void i2sInit() {
    i2s_config_t i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = I2S_READ_SAMPLES,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pinConfig = {
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_PIN
    };

    i2s_driver_install(I2S_MIC_PORT, &i2sConfig, 0, NULL);
    i2s_set_pin(I2S_MIC_PORT, &pinConfig);
}

static void sdInit() {
    SPIClass sdSpi(HSPI);
    sdSpi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    sdReady = SD.begin(SD_CS_PIN, sdSpi);
    if (!sdReady) {
        Serial.println("[SD] Karta niedostepna - nagrywanie na SD wylaczone");
    } else {
        Serial.println("[SD] Karta gotowa");
    }
}

static void startRecording() {
    if (!sdReady || recording) return;

    char path[32];
    snprintf(path, sizeof(path), "/rec_%03lu.wav", (unsigned long)recordingIndex++);
    recordFile = SD.open(path, FILE_WRITE);
    if (!recordFile) {
        Serial.printf("[SD] Nie udalo sie otworzyc %s\n", path);
        return;
    }
    wavWriteHeader(recordFile, SAMPLE_RATE);
    dataBytesWritten = 0;
    recording = true;
    Serial.printf("[REC] Start -> %s\n", path);
}

static void stopRecording() {
    if (!recording) return;
    wavFinalize(recordFile, dataBytesWritten);
    recordFile.close();
    recording = false;
    Serial.printf("[REC] Stop (%lu bajtow audio)\n", (unsigned long)dataBytesWritten);
}

static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                       AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] Klient #%u polaczony (czeka na autoryzacje)\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        authorizedClients.erase(client->id());
        Serial.printf("[WS] Klient #%u rozlaczony\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->opcode != WS_TEXT) return;
        String msg((char*)data, len);

        if (msg == String("AUTH:") + WS_AUTH_TOKEN) {
            authorizedClients.insert(client->id());
            Serial.printf("[WS] Klient #%u autoryzowany\n", client->id());
            return;
        }

        // Kazda inna komenda wymaga wczesniejszej autoryzacji.
        if (!authorizedClients.count(client->id())) {
            Serial.printf("[WS] Klient #%u odrzucony - brak/zly token\n", client->id());
            client->close(1008, "unauthorized");
            return;
        }

        if (msg == "REC:START") startRecording();
        else if (msg == "REC:STOP") stopRecording();
    }
}

void setup() {
    Serial.begin(115200);
    delay(300);

    if (!LittleFS.begin(true)) {
        Serial.println("[FS] Blad montowania LittleFS");
    }

    sdInit();
    i2sInit();

    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, WIFI_AP_HIDDEN, WIFI_AP_MAX_CONN);
    Serial.print("[WiFi] AP uruchomiony, IP: ");
    Serial.println(WiFi.softAPIP());

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.begin();

    Serial.println("[HTTP] Serwer uruchomiony");
}

void loop() {
    size_t bytesRead = 0;
    i2s_read(I2S_MIC_PORT, i2sRawBuf, sizeof(i2sRawBuf), &bytesRead, portMAX_DELAY);
    size_t samplesRead = bytesRead / sizeof(int32_t);

    for (size_t i = 0; i < samplesRead; i++) {
        int32_t s = i2sRawBuf[i] >> AUDIO_SHIFT_BITS;
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        pcmBuf[i] = (int16_t)s;
    }

    size_t pcmBytes = samplesRead * sizeof(int16_t);

    if (!authorizedClients.empty()) {
        for (auto *c : ws.getClients()) {
            if (authorizedClients.count(c->id())) {
                c->binary((uint8_t*)pcmBuf, pcmBytes);
            }
        }
    }

    if (recording) {
        recordFile.write((const uint8_t*)pcmBuf, pcmBytes);
        dataBytesWritten += pcmBytes;
        // okresowy flush, zeby nie stracic danych przy zaniku zasilania
        if ((dataBytesWritten % (pcmBytes * 32)) == 0) {
            recordFile.flush();
        }
    }

    ws.cleanupClients();
}
