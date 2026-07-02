#ifndef MEMO_FAVORITES_H
#define MEMO_FAVORITES_H

#include "memo_types.h"

#include <set>
#include <string>

class MemoFavorites {
public:
    void Clear();
    void Parse(const std::string &data);
    std::string Serialize() const;
    bool IsStarred(const std::string &path) const;
    void SetStarred(const std::string &path, bool starred);
    void Toggle(const std::string &path);
    void ApplyTo(MemoMetadata &memo) const;

private:
    std::set<std::string> starred_paths_;
};

#endif
