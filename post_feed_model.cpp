// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#include "post_feed_model.h"
#include <QQmlEngine>
#include <QQmlListProperty>

using namespace std::chrono_literals;

namespace Skywalker {

PostFeedModel::PostFeedModel(QObject* parent) :
    QAbstractListModel(parent)
{}

void PostFeedModel::addFeed(ATProto::AppBskyFeed::PostFeed&& feed)
{
    mRawFeed = std::forward<ATProto::AppBskyFeed::PostFeed>(feed);
    const int oldSize = mFeed.size();

    for (const auto& feedEntry : mRawFeed)
    {
        if (feedEntry->mPost->mRecordType == ATProto::RecordType::APP_BSKY_FEED_POST)
            mFeed.push_back(Post(feedEntry.get()));
        else
            qWarning() << "Unsupported post record type:" << int(feedEntry->mPost->mRecordType);
    }

    beginInsertRows({}, oldSize, mFeed.size() - 1);
    endInsertRows();
}

int PostFeedModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return mFeed.size();
}

QVariant PostFeedModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= mFeed.size())
        return {};

    const auto& post = mFeed[index.row()];

    switch (Role(role))
    {
    case Role::AuthorName:
        return post.getAuthor().getName();
    case Role::AuthorAvatar:
        return post.getAuthor().getAvatarUrl();
    case Role::PostText:
        return post.getText();
    case Role::PostCreatedSecondsAgo:
    {
        const auto duration = QDateTime::currentDateTime() - post.getCreatedAt();
        return qint64(duration / 1000ms);
    }
    case Role::PostImages:
    {
        QVariantList images;
        for (const auto& img : post.getImages())
            images.push_back(QVariant::fromValue(*img));

        qDebug() << "MICHEL:" << images.size();
        return images;
    }
    default:
        qDebug() << "Uknown role requested:" << role;
        break;
    }

    return {};
}

QHash<int, QByteArray> PostFeedModel::roleNames() const
{
    static const QHash<int, QByteArray> roles{
        { int(Role::AuthorName), "authorName" },
        { int(Role::AuthorAvatar), "authorAvatar" },
        { int(Role::PostText), "postText" },
        { int(Role::PostCreatedSecondsAgo), "postCreatedSecondsAgo" },
        { int(Role::PostImages), "postImages" },
    };

    return roles;
}

}
