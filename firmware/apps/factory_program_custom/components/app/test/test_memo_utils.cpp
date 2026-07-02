#include "memo_utils.h"
#include "audio_loop_policy.h"
#include "memo_storage.h"
#include "touch_gesture.h"
#include "power_sleep_policy.h"
#include "ui_sfx.h"
#include "voice_settings.h"
#include "memo_favorites.h"

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
    assert(format.sample_rate == 16000);
    assert(format.bits_per_sample == 16);
    assert(format.channels == 2);

    const uint32_t data_size = 64000;
    WavHeader header = MakeWavHeader(format, data_size);

    assert(std::memcmp(header.riff, "RIFF", 4) == 0);
    assert(std::memcmp(header.wave, "WAVE", 4) == 0);
    assert(std::memcmp(header.fmt, "fmt ", 4) == 0);
    assert(std::memcmp(header.data, "data", 4) == 0);
    assert(header.audio_format == 1);
    assert(header.sample_rate == 16000);
    assert(header.bits_per_sample == 16);
    assert(header.channels == 2);
    assert(header.block_align == 4);
    assert(header.byte_rate == 64000);
    assert(header.data_size == data_size);
    assert(header.file_size == data_size + sizeof(WavHeader) - 8);
    assert(BytesToDurationSeconds(data_size, format) == 1);
}

static void TestMemoSequenceHelpers() {
    assert(ParseMemoSequence("MEMO_0007_0904.wav") == 7);
    assert(ParseMemoSequence("MEMO_0007.wav") == 7);
    assert(ParseMemoSequence("bad.wav") == -1);
    assert(NextMemoSequence(std::vector<uint32_t>{1, 2, 4}) == 5);
    assert(NextMemoSequence(std::vector<uint32_t>{}) == 1);

    std::vector<MemoMetadata> memos(2);
    memos[0].path = "/flash/memos/MEMO_0001.wav";
    memos[1].path = "/flash/memos/MEMO_0002.wav";
    assert(FindMemoIndexByPath(memos, "/flash/memos/MEMO_0002.wav") == 1);
    assert(FindMemoIndexByPath(memos, "/flash/memos/MEMO_9999.wav") == -1);
}

static void TestMemoStorageBaseDirSwitching() {
    MemoStorage storage("/sdcard/memos");
    assert(storage.BuildPathForSequence(3, false, 0, 0) == "/sdcard/memos/MEMO_0003.wav");

    storage.SetBaseDir("/flash/memos");
    assert(storage.BaseDir() == "/flash/memos");
    assert(storage.BuildPathForSequence(4, true, 8, 5) == "/flash/memos/MEMO_0004_0805.wav");
}

static void TestTouchGestureClassification() {
    VoiceUiAction tap = ClassifyTouchGesture(40, 44, 42, 47, 3, 4, VoiceAppState::List);
    assert(tap.type == VoiceUiActionType::SelectRow);
    assert(tap.row == 0);

    VoiceUiAction drag = ClassifyTouchGesture(40, 44, 43, 58, 4, 14, VoiceAppState::List);
    assert(drag.type == VoiceUiActionType::None);

    VoiceUiAction next = ClassifyTouchGesture(40, 126, 39, 96, 2, 30, VoiceAppState::List);
    assert(next.type == VoiceUiActionType::NextPage);

    VoiceUiAction previous = ClassifyTouchGesture(40, 78, 41, 106, 2, 28, VoiceAppState::List);
    assert(previous.type == VoiceUiActionType::PreviousPage);

    VoiceUiAction detail_play = ClassifyTouchGesture(30, 160, 31, 161, 1, 1, VoiceAppState::Detail);
    assert(detail_play.type == VoiceUiActionType::PlayStop);

    VoiceUiAction detail_delete = ClassifyTouchGesture(100, 160, 101, 161, 1, 1, VoiceAppState::Detail);
    assert(detail_delete.type == VoiceUiActionType::Delete);

    VoiceUiAction detail_back = ClassifyTouchGesture(170, 160, 171, 161, 1, 1, VoiceAppState::Detail);
    assert(detail_back.type == VoiceUiActionType::Back);

    VoiceUiAction confirm_delete = ClassifyTouchGesture(50, 160, 51, 161, 1, 1, VoiceAppState::DeleteConfirm);
    assert(confirm_delete.type == VoiceUiActionType::ConfirmDelete);

    VoiceUiAction cancel_delete = ClassifyTouchGesture(150, 160, 151, 161, 1, 1, VoiceAppState::DeleteConfirm);
    assert(cancel_delete.type == VoiceUiActionType::Cancel);
}

static void TestAudioLoopPolicy() {
    assert(AppEventWaitTimeoutMs(false, false) == 50);
    assert(AppEventWaitTimeoutMs(true, false) == 50);
    assert(AppEventWaitTimeoutMs(false, true) == 0);
    assert(ShouldRefreshAudioProgress(0, UINT32_MAX));
    assert(ShouldRefreshAudioProgress(1, 0));
    assert(ShouldRefreshAudioProgress(2, 1));
    assert(!ShouldRefreshAudioProgress(2, 2));
    assert(SavingAnimationFrame(0) == 0);
    assert(SavingAnimationFrame(1) == 1);
    assert(SavingAnimationFrame(2) == 2);
    assert(SavingAnimationFrame(3) == 3);
    assert(SavingAnimationFrame(4) == 0);
}

static void TestPowerSleepPolicy() {
    assert(!ShouldEnterPowerButtonWakeSleep(false));
    assert(ShouldEnterPowerButtonWakeSleep(true));
}

static void TestUiSfxPolicy() {
    UiSfxSpec tap = UiSfxSpecForEvent(UiSfxEvent::Touch);
    assert(tap.frequency_hz > 0);
    assert(tap.duration_ms <= 60);
    assert(tap.post_delay_ms == 0);

    UiSfxSpec record_start = UiSfxSpecForEvent(UiSfxEvent::RecordStart);
    assert(record_start.duration_ms <= 70);
    assert(record_start.post_delay_ms >= 100);
    assert(record_start.discard_record_ms >= 200);

    UiSfxSpec error = UiSfxSpecForEvent(UiSfxEvent::Error);
    assert(error.duration_ms > tap.duration_ms);

    int16_t samples[64] = {};
    FillUiSfxPcm(tap, samples, 32);
    bool has_signal = false;
    for (size_t i = 0; i < 64; i += 2) {
        assert(samples[i] == samples[i + 1]);
        assert(samples[i] <= tap.amplitude);
        assert(samples[i] >= -tap.amplitude);
        has_signal = has_signal || samples[i] != 0;
    }
    assert(has_signal);
}

static void TestVoiceSettingsPolicy() {
    VoiceSettings settings;
    assert(settings.sfx_enabled);
    assert(settings.sfx_volume == UiSfxVolume::Medium);
    assert(!settings.record_start_sfx_enabled);
    assert(settings.record_mode == VoiceRecordMode::Hold);

    ApplyVoiceSettingsItem(&settings, VoiceSettingsItem::Sound);
    assert(!settings.sfx_enabled);
    ApplyVoiceSettingsItem(&settings, VoiceSettingsItem::Sound);
    assert(settings.sfx_enabled);

    ApplyVoiceSettingsItem(&settings, VoiceSettingsItem::Volume);
    assert(settings.sfx_volume == UiSfxVolume::High);
    ApplyVoiceSettingsItem(&settings, VoiceSettingsItem::Volume);
    assert(settings.sfx_volume == UiSfxVolume::Low);
    ApplyVoiceSettingsItem(&settings, VoiceSettingsItem::Volume);
    assert(settings.sfx_volume == UiSfxVolume::Medium);

    ApplyVoiceSettingsItem(&settings, VoiceSettingsItem::RecordStartSound);
    assert(settings.record_start_sfx_enabled);
    ApplyVoiceSettingsItem(&settings, VoiceSettingsItem::RecordMode);
    assert(settings.record_mode == VoiceRecordMode::Tap);
    ApplyVoiceSettingsItem(&settings, VoiceSettingsItem::RecordMode);
    assert(settings.record_mode == VoiceRecordMode::Hold);
    assert(VoiceSettingsItemForRow(3) == VoiceSettingsItem::RecordMode);
    assert(VoiceSettingsItemForRow(4) == VoiceSettingsItem::ClearAll);
    assert(std::string(VoiceSettingsVolumeLabel(UiSfxVolume::High)) == "HIGH");
    assert(std::string(VoiceSettingsRecordModeLabel(VoiceRecordMode::Tap)) == "TAP");
}

static void TestMemoFavoritesPolicy() {
    MemoMetadata memo;
    memo.path = "/flash/memos/MEMO_0001.wav";
    assert(!memo.starred);

    MemoFavorites favorites;
    assert(!favorites.IsStarred(memo.path));
    favorites.Toggle(memo.path);
    assert(favorites.IsStarred(memo.path));
    favorites.ApplyTo(memo);
    assert(memo.starred);
    favorites.Toggle(memo.path);
    assert(!favorites.IsStarred(memo.path));

    favorites.SetStarred("/flash/memos/MEMO_0002.wav", true);
    std::string serialized = favorites.Serialize();
    MemoFavorites loaded;
    loaded.Parse(serialized);
    assert(loaded.IsStarred("/flash/memos/MEMO_0002.wav"));
}

int main() {
    TestDurationFormatting();
    TestFilenameGeneration();
    TestPaging();
    TestWavHeader();
    TestMemoSequenceHelpers();
    TestMemoStorageBaseDirSwitching();
    TestTouchGestureClassification();
    TestAudioLoopPolicy();
    TestPowerSleepPolicy();
    TestUiSfxPolicy();
    TestVoiceSettingsPolicy();
    TestMemoFavoritesPolicy();
    std::cout << "memo_utils tests passed\n";
    return 0;
}
