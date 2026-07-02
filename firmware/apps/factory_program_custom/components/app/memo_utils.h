#ifndef MEMO_UTILS_H
#define MEMO_UTILS_H

#include "memo_types.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct __attribute__((packed)) WavHeader {
    char riff[4];
    uint32_t file_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
};

static_assert(sizeof(WavHeader) == 44, "WAV header must be 44 bytes");

WavHeader MakeWavHeader(const WavFormat &format, uint32_t data_size);
bool PatchWavSizes(FILE *file, uint32_t data_size);
std::string FormatDuration(uint32_t seconds);
std::string BuildMemoFilename(const std::string &base_dir, uint32_t sequence, bool has_time, uint8_t hour, uint8_t minute);
uint32_t CalculatePageCount(uint32_t item_count, uint32_t page_size);
uint32_t ClampPage(uint32_t page, uint32_t page_count);
uint32_t BytesToDurationSeconds(uint32_t data_bytes, const WavFormat &format);
bool ReadWavHeader(FILE *file, WavHeader *header);
bool IsSupportedWavHeader(const WavHeader &header, const WavFormat &format);
int32_t ParseMemoSequence(const std::string &filename);
uint32_t NextMemoSequence(const std::vector<uint32_t> &sequences);
int32_t FindMemoIndexByPath(const std::vector<MemoMetadata> &memos, const std::string &path);

#endif
