#include "memo_utils.h"

#include <cstring>
#include <iomanip>
#include <sstream>

WavHeader MakeWavHeader(const WavFormat &format, uint32_t data_size) {
    WavHeader header = {};
    std::memcpy(header.riff, "RIFF", 4);
    header.file_size = data_size + sizeof(WavHeader) - 8;
    std::memcpy(header.wave, "WAVE", 4);
    std::memcpy(header.fmt, "fmt ", 4);
    header.fmt_size = 16;
    header.audio_format = 1;
    header.channels = format.channels;
    header.sample_rate = format.sample_rate;
    header.bits_per_sample = format.bits_per_sample;
    header.block_align = (format.channels * format.bits_per_sample) / 8;
    header.byte_rate = format.sample_rate * header.block_align;
    std::memcpy(header.data, "data", 4);
    header.data_size = data_size;
    return header;
}

bool PatchWavSizes(FILE *file, uint32_t data_size) {
    if (file == nullptr) {
        return false;
    }

    uint32_t file_size = data_size + sizeof(WavHeader) - 8;
    if (std::fseek(file, 4, SEEK_SET) != 0) {
        return false;
    }
    if (std::fwrite(&file_size, sizeof(file_size), 1, file) != 1) {
        return false;
    }
    if (std::fseek(file, 40, SEEK_SET) != 0) {
        return false;
    }
    if (std::fwrite(&data_size, sizeof(data_size), 1, file) != 1) {
        return false;
    }
    return std::fflush(file) == 0;
}

std::string FormatDuration(uint32_t seconds) {
    uint32_t minutes = seconds / 60;
    uint32_t remaining_seconds = seconds % 60;
    std::ostringstream out;
    out << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << remaining_seconds;
    return out.str();
}

std::string BuildMemoFilename(const std::string &base_dir, uint32_t sequence, bool has_time, uint8_t hour, uint8_t minute) {
    std::ostringstream out;
    out << base_dir;
    if (!base_dir.empty() && base_dir.back() != '/') {
        out << "/";
    }
    out << "MEMO_" << std::setw(4) << std::setfill('0') << sequence;
    if (has_time) {
        out << "_" << std::setw(2) << std::setfill('0') << static_cast<int>(hour)
            << std::setw(2) << std::setfill('0') << static_cast<int>(minute);
    }
    out << ".wav";
    return out.str();
}

uint32_t CalculatePageCount(uint32_t item_count, uint32_t page_size) {
    if (page_size == 0 || item_count == 0) {
        return 1;
    }
    return (item_count + page_size - 1) / page_size;
}

uint32_t ClampPage(uint32_t page, uint32_t page_count) {
    if (page_count == 0) {
        return 0;
    }
    if (page >= page_count) {
        return page_count - 1;
    }
    return page;
}

uint32_t BytesToDurationSeconds(uint32_t data_bytes, const WavFormat &format) {
    uint32_t block_align = (format.channels * format.bits_per_sample) / 8;
    uint32_t byte_rate = format.sample_rate * block_align;
    if (byte_rate == 0) {
        return 0;
    }
    return data_bytes / byte_rate;
}

bool ReadWavHeader(FILE *file, WavHeader *header) {
    if (file == nullptr || header == nullptr) {
        return false;
    }
    if (std::fseek(file, 0, SEEK_SET) != 0) {
        return false;
    }
    return std::fread(header, sizeof(WavHeader), 1, file) == 1;
}

bool IsSupportedWavHeader(const WavHeader &header, const WavFormat &format) {
    return std::memcmp(header.riff, "RIFF", 4) == 0 &&
           std::memcmp(header.wave, "WAVE", 4) == 0 &&
           std::memcmp(header.fmt, "fmt ", 4) == 0 &&
           std::memcmp(header.data, "data", 4) == 0 &&
           header.audio_format == 1 &&
           header.sample_rate == format.sample_rate &&
           header.bits_per_sample == format.bits_per_sample &&
           header.channels == format.channels;
}
