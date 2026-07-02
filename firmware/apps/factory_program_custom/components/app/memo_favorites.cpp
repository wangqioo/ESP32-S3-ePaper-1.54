#include "memo_favorites.h"

void MemoFavorites::Clear() {
    starred_paths_.clear();
}

void MemoFavorites::Parse(const std::string &data) {
    starred_paths_.clear();
    size_t start = 0;
    while (start < data.size()) {
        size_t end = data.find('\n', start);
        if (end == std::string::npos) {
            end = data.size();
        }
        std::string path = data.substr(start, end - start);
        if (!path.empty()) {
            starred_paths_.insert(path);
        }
        start = end + 1;
    }
}

std::string MemoFavorites::Serialize() const {
    std::string data;
    for (const std::string &path : starred_paths_) {
        data += path;
        data += '\n';
    }
    return data;
}

bool MemoFavorites::IsStarred(const std::string &path) const {
    return starred_paths_.find(path) != starred_paths_.end();
}

void MemoFavorites::SetStarred(const std::string &path, bool starred) {
    if (path.empty()) {
        return;
    }
    if (starred) {
        starred_paths_.insert(path);
    } else {
        starred_paths_.erase(path);
    }
}

void MemoFavorites::Toggle(const std::string &path) {
    SetStarred(path, !IsStarred(path));
}

void MemoFavorites::ApplyTo(MemoMetadata &memo) const {
    memo.starred = IsStarred(memo.path);
}
