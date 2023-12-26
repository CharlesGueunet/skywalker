// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#pragma once
#include <QHashFunctions>
#include <QString>
#include <optional>
#include <unordered_map>

namespace Skywalker {

class  LocalFeedModelChanges
{
public:
    struct Change
    {
        int mLikeCountDelta = 0;

        // Not-set means not changed.
        // Empty means like removed.
        std::optional<QString> mLikeUri;
    };

    LocalFeedModelChanges() = default;
    virtual ~LocalFeedModelChanges() = default;

    const Change* getLocalChange(const QString& cid) const;
    void clearLocalChanges();

    void updateLikeCountDelta(const QString& cid, int delta);
    void updateLikeUri(const QString& cid, const QString& likeUri);

protected:
    virtual void likeCountChanged() = 0;
    virtual void likeUriChanged() = 0;

private:
    // Mapping from post CID to change
    std::unordered_map<QString, Change> mChanges;
};

}
