#include "memo_storage.h"

#include "memo_utils.h"
#include "port_sdcard.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

MemoStorage::MemoStorage(std::string base_dir) : base_dir_(std::move(base_dir)) {
}

esp_err_t MemoStorage::Init() {
    if (card == nullptr) {
        return ESP_ERR_NOT_FOUND;
    }

    struct stat st = {};
    if (stat(base_dir_.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode) ? ESP_OK : ESP_ERR_INVALID_STATE;
    }

    if (mkdir(base_dir_.c_str(), 0775) == 0) {
        return ESP_OK;
    }

    return errno == EEXIST ? ESP_OK : ESP_FAIL;
}

esp_err_t MemoStorage::Scan(std::vector<MemoMetadata> &out, const WavFormat &format) {
    out.clear();
    if (card == nullptr) {
        return ESP_ERR_NOT_FOUND;
    }

    DIR *dir = opendir(base_dir_.c_str());
    if (dir == nullptr) {
        return ESP_ERR_NOT_FOUND;
    }

    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        int32_t sequence = ParseMemoSequence(entry->d_name);
        if (sequence < 0) {
            continue;
        }

        std::string path = base_dir_ + "/" + entry->d_name;
        FILE *file = fopen(path.c_str(), "rb");
        if (file == nullptr) {
            continue;
        }

        WavHeader header = {};
        bool valid = ReadWavHeader(file, &header) && IsSupportedWavHeader(header, format);
        fclose(file);
        if (!valid) {
            continue;
        }

        MemoMetadata memo;
        memo.sequence = static_cast<uint32_t>(sequence);
        memo.path = path;
        memo.duration_seconds = BytesToDurationSeconds(header.data_size, format);
        memo.data_bytes = header.data_size;
        memo.display_time = "--:--";

        const char *name = entry->d_name;
        size_t name_len = strlen(name);
        if (name_len >= 18 && name[9] == '_' && name[14] == '.') {
            memo.display_time.assign(name + 10, 2);
            memo.display_time += ":";
            memo.display_time.append(name + 12, 2);
        }

        out.push_back(memo);
    }

    closedir(dir);
    std::sort(out.begin(), out.end(), [](const MemoMetadata &a, const MemoMetadata &b) {
        return a.sequence < b.sequence;
    });
    return ESP_OK;
}

uint32_t MemoStorage::NextSequence(const std::vector<MemoMetadata> &memos) const {
    std::vector<uint32_t> sequences;
    sequences.reserve(memos.size());
    for (const MemoMetadata &memo : memos) {
        sequences.push_back(memo.sequence);
    }
    return NextMemoSequence(sequences);
}

std::string MemoStorage::BuildPathForSequence(uint32_t sequence, bool has_time, uint8_t hour, uint8_t minute) const {
    return BuildMemoFilename(base_dir_, sequence, has_time, hour, minute);
}

void MemoStorage::RemoveFile(const char *path) const {
    if (path != nullptr) {
        unlink(path);
    }
}

const std::string &MemoStorage::BaseDir() const {
    return base_dir_;
}
