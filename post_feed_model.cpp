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

void PostFeedModel::setFeed(ATProto::AppBskyFeed::OutputFeed::Ptr&& feed)
{
    if (feed->mFeed.empty())
    {
        qDebug() << "Trying to set an empty feed";
        return;
    }

    if (mFeed.empty())
    {
        addFeed(std::forward<ATProto::AppBskyFeed::OutputFeed::Ptr>(feed));
        return;
    }

    clear();
    addFeed(std::forward<ATProto::AppBskyFeed::OutputFeed::Ptr>(feed));
}

int PostFeedModel::prependFeed(ATProto::AppBskyFeed::OutputFeed::Ptr&& feed)
{
    if (feed->mFeed.empty())
        return 0;

    if (mFeed.empty())
    {
        setFeed(std::forward<ATProto::AppBskyFeed::OutputFeed::Ptr>(feed));
        return 0;
    }

    auto page = createPage(std::forward<ATProto::AppBskyFeed::OutputFeed::Ptr>(feed));

    if (page->mFeed.empty())
    {
        qWarning() << "Page has no posts";
        return 0;
    }

    const auto overlapStart = findOverlapStart(*page, 0);

    if (!overlapStart)
    {
        page->mFeed.push_back(Post::createGapPlaceHolder(page->mCursorNextPage));
        const int gapId = page->mFeed.back().getGapId();

        const size_t lastInsertIndex = page->mFeed.size() - 1;
        beginInsertRows({}, 0, lastInsertIndex);
        mFeed.insert(mFeed.begin(), page->mFeed.begin(), page->mFeed.end());
        addToIndices(page->mFeed.size(), 0);
        mGapIdIndexMap[gapId] = lastInsertIndex;

        // The -1 offset on lastInsertIndex is for the place holder post.
        if (!page->mCursorNextPage.isEmpty())
            mIndexCursorMap[lastInsertIndex - 1] = page->mCursorNextPage;

        mIndexRawFeedMap[lastInsertIndex - 1] = std::move(page->mRawFeed);
        endInsertRows();

        qDebug() << "Full feed prepended, new size:" << mFeed.size();
        logIndices();
        return gapId;
    }

    if (*overlapStart == 0)
    {
        qDebug() << "Full overlap, no new posts";
        return 0;
    }

    const auto overlapEnd = findOverlapEnd(*page, 0);

    beginInsertRows({}, 0, *overlapStart - 1);
    mFeed.insert(mFeed.begin(), page->mFeed.begin(), page->mFeed.begin() + *overlapStart);
    addToIndices(*overlapStart, 0);

    if (!page->mCursorNextPage.isEmpty() && overlapEnd)
        mIndexCursorMap[*overlapStart + *overlapEnd] = page->mCursorNextPage;

    mIndexRawFeedMap[*overlapStart - 1] = std::move(page->mRawFeed);
    endInsertRows();

    qDebug() << "Prepended" << *overlapStart << "posts, new size:" << mFeed.size();
    logIndices();
    return 0;
}

int PostFeedModel::gapFillFeed(ATProto::AppBskyFeed::OutputFeed::Ptr&& feed, int gapId)
{
    qDebug() << "Fill gap:" << gapId;

    if (!mGapIdIndexMap.count(gapId))
    {
        qWarning() << "Gap does not exist:" << gapId;
        return 0;
    }

    const int gapIndex = mGapIdIndexMap[gapId];
    mGapIdIndexMap.erase(gapId);

    if (gapIndex > mFeed.size())
    {
        qWarning() << "Gap:" << gapId << "index:" << gapIndex << "beyond feed size" << mFeed.size();
        return 0;
    }

    Q_ASSERT(mFeed[gapIndex].getGapId() == gapId);

    beginRemoveRows({}, gapIndex, gapIndex);
    mFeed.erase(mFeed.begin() + gapIndex);
    addToIndices(-1, gapIndex);
    endRemoveRows();

    qDebug() << "Removed place holder post:" << gapIndex;
    logIndices();

    auto page = createPage(std::forward<ATProto::AppBskyFeed::OutputFeed::Ptr>(feed));

    if (page->mFeed.empty())
    {
        qWarning() << "Page has no posts";
        return 0;
    }

    const auto overlapStart = findOverlapStart(*page, gapIndex);

    if (!overlapStart)
    {
        page->mFeed.push_back(Post::createGapPlaceHolder(page->mCursorNextPage));
        const int gapId = page->mFeed.back().getGapId();

        const size_t lastInsertIndex = gapIndex + page->mFeed.size() - 1;
        beginInsertRows({}, gapIndex, lastInsertIndex);
        mFeed.insert(mFeed.begin() + gapIndex, page->mFeed.begin(), page->mFeed.end());
        addToIndices(page->mFeed.size(), gapIndex);
        mGapIdIndexMap[gapId] = lastInsertIndex;

        // The -1 offset on lastInsertIndex is for the place holder post.
        if (!page->mCursorNextPage.isEmpty())
            mIndexCursorMap[lastInsertIndex - 1] = page->mCursorNextPage;

        mIndexRawFeedMap[lastInsertIndex - 1] = std::move(page->mRawFeed);
        endInsertRows();

        qDebug() << "Full feed inserted in gap, new size:" << mFeed.size();
        logIndices();
        return gapId;
    }

    if (*overlapStart == 0)
    {
        qDebug() << "Full overlap, no new posts";
        return 0;
    }

    const auto overlapEnd = findOverlapEnd(*page, gapIndex);

    beginInsertRows({}, gapIndex, gapIndex + *overlapStart - 1);
    mFeed.insert(mFeed.begin(), page->mFeed.begin(), page->mFeed.begin() + *overlapStart);
    addToIndices(*overlapStart, gapIndex);

    if (!page->mCursorNextPage.isEmpty() && overlapEnd)
        mIndexCursorMap[*overlapStart + *overlapEnd] = page->mCursorNextPage;

    mIndexRawFeedMap[gapIndex + *overlapStart - 1] = std::move(page->mRawFeed);
    endInsertRows();

    qDebug() << "Inserted in gap" << *overlapStart << "posts, new size:" << mFeed.size();
    logIndices();
    return 0;
}

void PostFeedModel::clear()
{
    beginRemoveRows({}, 0, mFeed.size() - 1);
    mFeed.clear();
    mIndexCursorMap.clear();
    mIndexRawFeedMap.clear();
    mGapIdIndexMap.clear();
    endRemoveRows();
    qDebug() << "All posts removed";
}

void PostFeedModel::addFeed(ATProto::AppBskyFeed::OutputFeed::Ptr&& feed)
{
    qDebug() << "Add raw posts:" << feed->mFeed.size();

    if (feed->mFeed.empty())
        return;

    auto page = createPage(std::forward<ATProto::AppBskyFeed::OutputFeed::Ptr>(feed));

    if (page->mFeed.empty())
    {
        qWarning() << "Page has no posts";
        return;
    }

    const size_t newRowCount = mFeed.size() + page->mFeed.size();

    beginInsertRows({}, mFeed.size(), newRowCount - 1);
    mFeed.insert(mFeed.end(), page->mFeed.begin(), page->mFeed.end());

    if (!page->mCursorNextPage.isEmpty())
        mIndexCursorMap[mFeed.size() - 1] = page->mCursorNextPage;
    else
        mEndOfFeed = true;

    mIndexRawFeedMap[mFeed.size() - 1] = std::move(page->mRawFeed);
    endInsertRows();

    qDebug() << "New feed size:" << mFeed.size();
    logIndices();
}

QString PostFeedModel::getLastCursor() const
{
    if (mEndOfFeed || mIndexCursorMap.empty())
        return {};

    return mIndexCursorMap.rbegin()->second;
}

const Post* PostFeedModel::getGapPlaceHolder(int gapId) const
{
    const auto it = mGapIdIndexMap.find(gapId);

    if (it == mGapIdIndexMap.end())
    {
        qWarning() << "Gap does not exist:" << gapId;
        return nullptr;
    }

    const int gapIndex = it->second;

    if (gapIndex > mFeed.size())
    {
        qWarning() << "Gap:" << gapId << "index:" << gapIndex << "beyond feed size" << mFeed.size();
        return nullptr;
    }

    const auto* gap = &mFeed[gapIndex];
    Q_ASSERT(gap->getGapId() == gapId);
    return gap;
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
    case Role::Author:
        return QVariant::fromValue(post.getAuthor());
    case Role::PostText:
        return post.getText();
    case Role::PostIndexedSecondsAgo:
    {
        const auto duration = QDateTime::currentDateTime() - post.getIndexedAt();
        return qint64(duration / 1000ms);
    }
    case Role::PostImages:
    {
        QList<ImageView> images;
        for (const auto& img : post.getImages())
            images.push_back(*img);

        return QVariant::fromValue(images);
    }
    case Role::PostExternal:
    {
        auto external = post.getExternalView();
        return external ? QVariant::fromValue(*external) : QVariant();
    }
    case Role::PostRepostedByName:
    {
        const auto& repostedBy = post.getRepostedBy();
        return repostedBy ? repostedBy->getName() : QVariant();
    }
    case Role::PostRecord:
    {
        auto record = post.getRecordView();
        return record ? QVariant::fromValue(*record) : QVariant();
    }
    case Role::PostRecordWithMedia:
    {
        auto record = post.getRecordWithMediaView();
        return record ? QVariant::fromValue(*record) : QVariant();
    }
    case Role::PostGapId:
        return post.getGapId();
    case Role::EndOfFeed:
        return post.isEndOfFeed();
    default:
        qDebug() << "Uknown role requested:" << role;
        break;
    }

    return {};
}

QHash<int, QByteArray> PostFeedModel::roleNames() const
{
    static const QHash<int, QByteArray> roles{
        { int(Role::Author), "author" },
        { int(Role::PostText), "postText" },
        { int(Role::PostIndexedSecondsAgo), "postIndexedSecondsAgo" },
        { int(Role::PostRepostedByName), "postRepostedByName" },
        { int(Role::PostImages), "postImages" },
        { int(Role::PostExternal), "postExternal" },
        { int(Role::PostRecord), "postRecord" },
        { int(Role::PostRecordWithMedia), "postRecordWithMedia" },
        { int(Role::PostGapId), "postGapId" },
        { int(Role::EndOfFeed), "endOfFeed" }
    };

    return roles;
}

PostFeedModel::Page::Ptr PostFeedModel::createPage(ATProto::AppBskyFeed::OutputFeed::Ptr&& feed) const
{
    auto page = std::make_unique<Page>();
    page->mRawFeed = std::forward<ATProto::AppBskyFeed::PostFeed>(feed->mFeed);

    for (const auto& feedEntry : page->mRawFeed)
    {
        if (feedEntry->mPost->mRecordType == ATProto::RecordType::APP_BSKY_FEED_POST)
            page->mFeed.push_back(Post(feedEntry.get()));
        else
            qWarning() << "Unsupported post record type:" << int(feedEntry->mPost->mRecordType);
    }

    if (feed->mCursor && !feed->mCursor->isEmpty())
    {
        page->mCursorNextPage = *feed->mCursor;
    }
    else
    {
        if (!page->mFeed.empty())
            page->mFeed.back().setEndOfFeed(true);
    }

    return page;
}

std::optional<size_t> PostFeedModel::findOverlapStart(const Page& page, size_t feedIndex) const
{
    Q_ASSERT(mFeed.size() > feedIndex);
    Q_ASSERT(!mFeed[feedIndex].isPlaceHolder());
    const auto& cidFirstStoredPost = mFeed[feedIndex].getCid();
    const auto& timestampFirstStoredPost = mFeed[feedIndex].getTimelineTimestamp();

    for (size_t i = 0; i < page.mFeed.size(); ++i)
    {
        const auto& post = page.mFeed[i];

        // Check on timestamp because of repost.
        // A repost will have the cid of the original post.
        if (cidFirstStoredPost == post.getCid() && timestampFirstStoredPost == post.getTimelineTimestamp())
        {
            qDebug() << "Matching overlap index found:" << i;
            return i;
        }

        if (timestampFirstStoredPost > post.getTimelineTimestamp())
        {
            qDebug() << "Overlap on timestamp found:" << i << timestampFirstStoredPost << post.getTimelineTimestamp();
            return i;
        }
    }

    // NOTE: the gap may be empty when the last post in the page is the predecessor of
    // the first post the stored feed. There is no way of knowing.
    qDebug() << "No overlap found, there is a gap";
    return {};
}

std::optional<size_t> PostFeedModel::findOverlapEnd(const Page& page, size_t feedIndex) const
{
    Q_ASSERT(!page.mFeed.empty());
    const auto& cidLastPagePost = page.mFeed.back().getCid();
    const auto& timestampLastPagePost = page.mFeed.back().getTimelineTimestamp();

    for (size_t i = feedIndex; i < mFeed.size(); ++ i)
    {
        const auto& post = mFeed[i];

        if (post.isPlaceHolder())
            continue;

        if (cidLastPagePost == post.getCid() && timestampLastPagePost == post.getTimelineTimestamp())
        {
            qDebug() << "Last matching overlap index found:" << i;
            return i;
        }

        if (timestampLastPagePost > post.getTimelineTimestamp())
        {
            qDebug() << "Overlap on timestamp found:" << i << timestampLastPagePost << post.getTimelineTimestamp();
            return i;
        }
    }

    qWarning() << "No overlap found, page exceeds end of stored feed";
    return {};
}

void PostFeedModel::addToIndices(size_t offset, size_t startAtIndex)
{
    std::map<size_t, QString> newCursorMap;
    for (const auto& [index, cursor] : mIndexCursorMap)
    {
        if (index >= startAtIndex)
            newCursorMap[index + offset] = cursor;
        else
            newCursorMap[index] = cursor;
    }

    mIndexCursorMap = std::move(newCursorMap);

    std::map<size_t, ATProto::AppBskyFeed::PostFeed> newRawFeedMap;
    for (auto& [index, rawFeed] : mIndexRawFeedMap)
    {
        if (index >= startAtIndex)
            newRawFeedMap[index + offset] = std::move(rawFeed);
        else
            newRawFeedMap[index] = std::move(rawFeed);
    }

    mIndexRawFeedMap = std::move(newRawFeedMap);

    for (auto& [gapId, index] : mGapIdIndexMap)
    {
        if (index >= startAtIndex)
            index += offset;
    }
}

void PostFeedModel::logIndices() const
{
    qDebug() << "INDEX CURSOR MAP:";
    for (const auto& [index, cursor] : mIndexCursorMap)
        qDebug() << "Index:" << index << "Cursor:" << cursor;

    qDebug() << "INDEX RAW FEED MAP:";
    for (const auto& [index, rawFeed] : mIndexRawFeedMap)
    {
        Q_ASSERT(!rawFeed.empty());
        qDebug() << "Index:" << index << "cid:" << rawFeed.front()->mPost->mCid
                 << "indexedAt:" << rawFeed.front()->mPost->mIndexedAt;
    }

    qDebug() << "GAP INDEX MAP:";
    for (const auto& [gapId, index] : mGapIdIndexMap)
        qDebug() << "Gap:" << gapId << "Index:" << index;
}

}
