#include "memo_recorder.h"

#include "memo_utils.h"
#include "port_codec.h"

#include <esp_heap_caps.h>
#include <esp_timer.h>

#include <cstring>
#include <unistd.h>

namespace {
constexpr size_t kMaxBufferedPcmBytes = 4 * 1024 * 1024;
}

MemoRecorder::MemoRecorder(size_t chunk_size) : chunk_size_(chunk_size) {
    buffer_ = static_cast<uint8_t *>(heap_caps_malloc(chunk_size_, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (buffer_ == nullptr) {
        buffer_ = static_cast<uint8_t *>(heap_caps_malloc(chunk_size_, MALLOC_CAP_8BIT));
    }
}

MemoRecorder::~MemoRecorder() {
    Abort();
    if (buffer_ != nullptr) {
        heap_caps_free(buffer_);
    }
    if (pcm_buffer_ != nullptr) {
        heap_caps_free(pcm_buffer_);
    }
}

esp_err_t MemoRecorder::Start(const char *path, const WavFormat &format) {
    if (path == nullptr || buffer_ == nullptr || IsRecording()) {
        return ESP_ERR_INVALID_STATE;
    }

    if (pcm_buffer_ != nullptr) {
        heap_caps_free(pcm_buffer_);
        pcm_buffer_ = nullptr;
    }

    pcm_capacity_ = kMaxBufferedPcmBytes - (kMaxBufferedPcmBytes % chunk_size_);
    pcm_buffer_ = static_cast<uint8_t *>(heap_caps_malloc(pcm_capacity_, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (pcm_buffer_ == nullptr) {
        pcm_capacity_ = 0;
        return ESP_ERR_NO_MEM;
    }

    format_ = format;
    path_ = path;
    data_bytes_ = 0;
    started_us_ = esp_timer_get_time();
    return ESP_OK;
}

esp_err_t MemoRecorder::CaptureChunk() {
    if (!IsRecording()) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = Codec_RecordData(buffer_, chunk_size_);
    if (ret != ESP_OK) {
        return ret;
    }

    if (data_bytes_ + chunk_size_ > pcm_capacity_) {
        return ESP_ERR_NO_MEM;
    }

    std::memcpy(pcm_buffer_ + data_bytes_, buffer_, chunk_size_);
    data_bytes_ += static_cast<uint32_t>(chunk_size_);
    return ESP_OK;
}

esp_err_t MemoRecorder::Stop(MemoMetadata *out_metadata) {
    if (!IsRecording()) {
        return ESP_ERR_INVALID_STATE;
    }

    file_ = fopen(path_.c_str(), "wb");
    if (file_ == nullptr) {
        if (pcm_buffer_ != nullptr) {
            heap_caps_free(pcm_buffer_);
            pcm_buffer_ = nullptr;
            pcm_capacity_ = 0;
        }
        path_.clear();
        data_bytes_ = 0;
        started_us_ = 0;
        return ESP_FAIL;
    }

    WavHeader header = MakeWavHeader(format_, data_bytes_);
    bool written = fwrite(&header, sizeof(header), 1, file_) == 1 &&
                   fwrite(pcm_buffer_, 1, data_bytes_, file_) == data_bytes_;
    fclose(file_);
    file_ = nullptr;

    if (pcm_buffer_ != nullptr) {
        heap_caps_free(pcm_buffer_);
        pcm_buffer_ = nullptr;
        pcm_capacity_ = 0;
    }

    if (!written) {
        unlink(path_.c_str());
        path_.clear();
        data_bytes_ = 0;
        return ESP_FAIL;
    }

    if (out_metadata != nullptr) {
        out_metadata->path = path_;
        out_metadata->duration_seconds = BytesToDurationSeconds(data_bytes_, format_);
        out_metadata->data_bytes = data_bytes_;
    }

    path_.clear();
    data_bytes_ = 0;
    started_us_ = 0;
    return ESP_OK;
}

void MemoRecorder::Abort() {
    if (file_ != nullptr) {
        fclose(file_);
        file_ = nullptr;
    }
    if (pcm_buffer_ != nullptr) {
        heap_caps_free(pcm_buffer_);
        pcm_buffer_ = nullptr;
        pcm_capacity_ = 0;
    }
    if (!path_.empty()) {
        unlink(path_.c_str());
    }
    path_.clear();
    data_bytes_ = 0;
    started_us_ = 0;
}

bool MemoRecorder::IsRecording() const {
    return pcm_buffer_ != nullptr && !path_.empty();
}

uint32_t MemoRecorder::ElapsedSeconds() const {
    if (!IsRecording() || started_us_ == 0) {
        return 0;
    }
    int64_t elapsed_us = esp_timer_get_time() - started_us_;
    return elapsed_us > 0 ? static_cast<uint32_t>(elapsed_us / 1000000) : 0;
}

uint32_t MemoRecorder::DataBytes() const {
    return data_bytes_;
}

const std::string &MemoRecorder::Path() const {
    return path_;
}
