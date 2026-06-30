#include "memo_utils.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

static void ExpectString(const char *actual, const char *expected) {
    assert(std::string(actual) == expected);
}

static void TestDurationFormatting() {
    ExpectString(FormatDuration(0).c_str(), "00:00");
    ExpectString(FormatDuration(83).c_str(), "01:23");
    ExpectString(FormatDuration(3723).c_str(), "62:03");
}

static void TestFilenameGeneration() {
    assert(BuildMemoFilename("/sdcard/memos", 7, true, 9, 4) == "/sdcard/memos/MEMO_0007_0904.wav");
    assert(BuildMemoFilename("/sdcard/memos", 7, false, 0, 0) == "/sdcard/memos/MEMO_0007.wav");
}

static void TestPaging() {
    assert(CalculatePageCount(0, 4) == 1);
    assert(CalculatePageCount(5, 4) == 2);
    assert(ClampPage(4, 2) == 1);
}

static void TestWavHeader() {
    WavFormat format;
    format.sample_rate = 16000;
    format.bits_per_sample = 16;
    format.channels = 2;

    const uint32_t data_size = 32000;
    WavHeader header = MakeWavHeader(format, data_size);

    assert(std::memcmp(header.riff, "RIFF", 4) == 0);
    assert(std::memcmp(header.wave, "WAVE", 4) == 0);
    assert(std::memcmp(header.fmt, "fmt ", 4) == 0);
    assert(std::memcmp(header.data, "data", 4) == 0);
    assert(header.audio_format == 1);
    assert(header.sample_rate == 16000);
    assert(header.bits_per_sample == 16);
    assert(header.channels == 2);
    assert(header.data_size == data_size);
    assert(header.file_size == data_size + sizeof(WavHeader) - 8);
}

static void TestMemoSequenceHelpers() {
    assert(ParseMemoSequence("MEMO_0007_0904.wav") == 7);
    assert(ParseMemoSequence("MEMO_0007.wav") == 7);
    assert(ParseMemoSequence("bad.wav") == -1);
    assert(NextMemoSequence(std::vector<uint32_t>{1, 2, 4}) == 5);
    assert(NextMemoSequence(std::vector<uint32_t>{}) == 1);
}

int main() {
    TestDurationFormatting();
    TestFilenameGeneration();
    TestPaging();
    TestWavHeader();
    TestMemoSequenceHelpers();
    std::cout << "memo_utils tests passed\n";
    return 0;
}
