#pragma once
#include <Arduino.h>
#include <FS.h>

// Minimalny zapis nagłówka WAV (PCM 16-bit mono) z możliwością
// późniejszej korekty rozmiarów po zamknięciu pliku.

struct WavHeader {
    char     riff[4]      = {'R','I','F','F'};
    uint32_t chunkSize    = 0; // uzupełniane przy zamknięciu
    char     wave[4]      = {'W','A','V','E'};
    char     fmt[4]       = {'f','m','t',' '};
    uint32_t fmtSize      = 16;
    uint16_t audioFormat  = 1; // PCM
    uint16_t numChannels  = 1;
    uint32_t sampleRate   = 16000;
    uint32_t byteRate     = 0;
    uint16_t blockAlign   = 0;
    uint16_t bitsPerSample= 16;
    char     data[4]      = {'d','a','t','a'};
    uint32_t dataSize     = 0; // uzupełniane przy zamknięciu
};

inline void wavWriteHeader(File &f, uint32_t sampleRate) {
    WavHeader h;
    h.sampleRate = sampleRate;
    h.byteRate = sampleRate * h.numChannels * (h.bitsPerSample / 8);
    h.blockAlign = h.numChannels * (h.bitsPerSample / 8);
    f.write((const uint8_t*)&h, sizeof(WavHeader));
}

// Wywołać tuż przed zamknięciem pliku, gdy znane jest dataBytesWritten.
inline void wavFinalize(File &f, uint32_t dataBytesWritten) {
    uint32_t chunkSize = 36 + dataBytesWritten;
    f.seek(4);
    f.write((const uint8_t*)&chunkSize, 4);
    f.seek(40);
    f.write((const uint8_t*)&dataBytesWritten, 4);
    f.seek(f.size());
}
