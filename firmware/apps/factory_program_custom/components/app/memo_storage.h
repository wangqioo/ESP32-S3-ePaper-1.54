#ifndef MEMO_STORAGE_H
#define MEMO_STORAGE_H

#include "memo_types.h"

#include <esp_err.h>

#include <string>
#include <vector>

class MemoStorage {
public:
    explicit MemoStorage(std::string base_dir = "/sdcard/memos");

    void SetBaseDir(std::string base_dir);
    esp_err_t Init();
    esp_err_t Scan(std::vector<MemoMetadata> &out, const WavFormat &format);
    uint32_t NextSequence(const std::vector<MemoMetadata> &memos) const;
    std::string BuildPathForSequence(uint32_t sequence, bool has_time, uint8_t hour, uint8_t minute) const;
    esp_err_t RemoveFile(const char *path) const;
    const std::string &BaseDir() const;

private:
    std::string base_dir_;
};

#endif
