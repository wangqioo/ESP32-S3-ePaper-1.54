#ifndef MEMO_RECORDER_H
#define MEMO_RECORDER_H

#include "memo_types.h"

#include <esp_err.h>

#include <cstdio>
#include <string>

class MemoRecorder {
public:
    explicit MemoRecorder(size_t chunk_size = 4096);
    ~MemoRecorder();

    esp_err_t Start(const char *path, const WavFormat &format);
    esp_err_t CaptureChunk();
    esp_err_t Stop(MemoMetadata *out_metadata);
    void Abort();
    bool IsRecording() const;
    uint32_t ElapsedSeconds() const;
    uint32_t DataBytes() const;
    const std::string &Path() const;

private:
    FILE *file_ = nullptr;
    uint8_t *buffer_ = nullptr;
    uint8_t *pcm_buffer_ = nullptr;
    size_t chunk_size_ = 0;
    size_t pcm_capacity_ = 0;
    WavFormat format_;
    std::string path_;
    uint32_t data_bytes_ = 0;
    int64_t started_us_ = 0;
};

#endif
