#include "memo_recorder.h"

#include "memo_utils.h"
#include "port_codec.h"

#include <esp_heap_caps.h>
#include <esp_timer.h>

#include <cstring>
#include <unistd.h>

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
}

esp_err_t MemoRecorder::Start(const char *path, const WavFormat &format) {
    if (path == nullptr || buffer_ == nullptr || IsRecording()) {
        return ESP_ERR_INVALID_STATE;
    }

    file_ = fopen(path, "wb+");
    if (file_ == nullptr) {
        return ESP_FAIL;
    }

    format_ = format;
    path_ = path;
    data_bytes_ = 0;
    started_us_ = esp_timer_get_time();

    WavHeader header = MakeWavHeader(format_, 0);
    if (fwrite(&header, sizeof(header), 1, file_) != 1) {
        Abort();
        return ESP_FAIL;
    }
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

    size_t written = fwrite(buffer_, 1, chunk_size_, file_);
    if (written != chunk_size_) {
        return ESP_FAIL;
    }
    data_bytes_ += static_cast<uint32_t>(written);
    return ESP_OK;
}

esp_err_t MemoRecorder::Stop(MemoMetadata *out_metadata) {
    if (!IsRecording()) {
        return ESP_ERR_INVALID_STATE;
    }

    bool patched = PatchWavSizes(file_, data_bytes_);
    fclose(file_);
    file_ = nullptr;

    if (!patched) {
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
    if (!path_.empty()) {
        unlink(path_.c_str());
    }
    path_.clear();
    data_bytes_ = 0;
    started_us_ = 0;
}

bool MemoRecorder::IsRecording() const {
    return file_ != nullptr;
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
