#ifndef MEMO_PLAYER_H
#define MEMO_PLAYER_H

#include "memo_types.h"

#include <esp_err.h>

#include <cstdio>

class MemoPlayer {
public:
    explicit MemoPlayer(size_t chunk_size = 4096);
    ~MemoPlayer();

    esp_err_t Start(const char *path, const WavFormat &expected_format);
    esp_err_t PlayChunk();
    void Stop();
    bool IsPlaying() const;
    uint32_t ElapsedSeconds() const;
    uint32_t DurationSeconds() const;

private:
    FILE *file_ = nullptr;
    uint8_t *buffer_ = nullptr;
    size_t chunk_size_ = 0;
    WavFormat format_;
    uint32_t data_bytes_ = 0;
    uint32_t played_bytes_ = 0;
};

#endif
