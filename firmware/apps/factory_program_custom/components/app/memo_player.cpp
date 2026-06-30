#include "memo_player.h"

#include "memo_utils.h"
#include "port_codec.h"

#include <esp_heap_caps.h>

MemoPlayer::MemoPlayer(size_t chunk_size) : chunk_size_(chunk_size) {
    buffer_ = static_cast<uint8_t *>(heap_caps_malloc(chunk_size_, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (buffer_ == nullptr) {
        buffer_ = static_cast<uint8_t *>(heap_caps_malloc(chunk_size_, MALLOC_CAP_8BIT));
    }
}

MemoPlayer::~MemoPlayer() {
    Stop();
    if (buffer_ != nullptr) {
        heap_caps_free(buffer_);
    }
}

esp_err_t MemoPlayer::Start(const char *path, const WavFormat &expected_format) {
    if (path == nullptr || buffer_ == nullptr || IsPlaying()) {
        return ESP_ERR_INVALID_STATE;
    }

    file_ = fopen(path, "rb");
    if (file_ == nullptr) {
        return ESP_FAIL;
    }

    WavHeader header = {};
    if (!ReadWavHeader(file_, &header) || !IsSupportedWavHeader(header, expected_format)) {
        Stop();
        return ESP_ERR_INVALID_ARG;
    }

    format_ = expected_format;
    data_bytes_ = header.data_size;
    played_bytes_ = 0;
    return ESP_OK;
}

esp_err_t MemoPlayer::PlayChunk() {
    if (!IsPlaying()) {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t remaining = data_bytes_ - played_bytes_;
    if (remaining == 0) {
        Stop();
        return ESP_OK;
    }

    size_t to_read = remaining < chunk_size_ ? remaining : chunk_size_;
    size_t read = fread(buffer_, 1, to_read, file_);
    if (read == 0) {
        Stop();
        return ESP_OK;
    }

    esp_err_t ret = Codec_PlaybackData(buffer_, read);
    if (ret != ESP_OK) {
        Stop();
        return ret;
    }

    played_bytes_ += static_cast<uint32_t>(read);
    if (played_bytes_ >= data_bytes_) {
        Stop();
    }
    return ESP_OK;
}

void MemoPlayer::Stop() {
    if (file_ != nullptr) {
        fclose(file_);
        file_ = nullptr;
    }
    data_bytes_ = 0;
    played_bytes_ = 0;
}

bool MemoPlayer::IsPlaying() const {
    return file_ != nullptr;
}

uint32_t MemoPlayer::ElapsedSeconds() const {
    return BytesToDurationSeconds(played_bytes_, format_);
}

uint32_t MemoPlayer::DurationSeconds() const {
    return BytesToDurationSeconds(data_bytes_, format_);
}
