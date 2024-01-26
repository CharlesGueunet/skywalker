// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#include "abstract_post_feed_model.h"
#include "content_filter.h"
#include <atproto/lib/post_master.h>

namespace Skywalker {

using namespace std::chrono_literals;

AbstractPostFeedModel::AbstractPostFeedModel(const QString& userDid, const IProfileStore& following,
                                             const ContentFilter& contentFilter, const Bookmarks& bookmarks,
                                             const MutedWords& mutedWords, QObject* parent) :
    QAbstractListModel(parent),
    mUserDid(userDid),
    mFollowing(following),
    mContentFilter(contentFilter),
    mBookmarks(bookmarks),
    mMutedWords(mutedWords)
{
    connect(&mBookmarks, &Bookmarks::sizeChanged, this, [this]{ postBookmarkedChanged(); });
}

void AbstractPostFeedModel::clearFeed()
{
    mFeed.clear();
    mStoredCids.clear();
    mStoredCidQueue = {};
    mEndOfFeed = false;
    clearLocalChanges();
    clearLocalProfileChanges();
}

void AbstractPostFeedModel::storeCid(const QString& cid)
{
    mStoredCids.insert(cid);
    mStoredCidQueue.push(cid);
}

void AbstractPostFeedModel::removeStoredCid(const QString& cid)
{
    // We leave the cid's in the chronological queues. They don't do
    // harm. At cleanup, the non-stored cid's will be removed from the queue.
    mStoredCids.erase(cid);
}

void AbstractPostFeedModel::cleanupStoredCids()
{
    while (mStoredCids.size() > MAX_TIMELINE_SIZE && !mStoredCidQueue.empty())
    {
        const auto& cid = mStoredCidQueue.front();
        mStoredCids.erase(cid);
        mStoredCidQueue.pop();
    }

    if (mStoredCidQueue.empty())
    {
        Q_ASSERT(mStoredCids.empty());
        qWarning() << "Stored cid set should have been empty:" << mStoredCids.size();
        mStoredCids.clear();
    }

    qDebug() << "Stored cid set:" << mStoredCids.size() << "cid queue:" << mStoredCidQueue.size();
}

bool AbstractPostFeedModel::mustHideContent(const Post& post) const
{
    if (post.getAuthor().getViewer().isMuted())
    {
        qDebug() << "Hide post of muted author:" << post.getAuthor().getHandleOrDid() << post.getCid();
        return true;
    }

    const auto [visibility, warning] = mContentFilter.getVisibilityAndWarning(post.getLabels());

    if (visibility == QEnums::CONTENT_VISIBILITY_HIDE_POST)
    {
        qDebug() << "Hide post:" << post.getCid() << warning;
        return true;
    }

    if (mMutedWords.match(post))
    {
        qDebug() << "Hide post due to muted words" << post.getCid();
        return true;
    }

    return false;
}

int AbstractPostFeedModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return mFeed.size();
}

QVariant AbstractPostFeedModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= (int)mFeed.size())
        return {};

    const auto& post = mFeed[index.row()];
    const auto* change = getLocalChange(post.getCid());

    switch (Role(role))
    {
    case Role::Author:
    {
        const auto author = post.getAuthor();
        const BasicProfile* profileChange = getProfileChange(author.getDid());
        return QVariant::fromValue(profileChange ? *profileChange : author);
    }
    case Role::PostText:
        return post.getFormattedText();
    case Role::PostPlainText:
        return post.getText();
    case Role::PostUri:
        return post.getUri();
    case Role::PostCid:
        return post.getCid();
    case Role::PostIndexedDateTime:
        return post.getIndexedAt();
    case Role::PostImages:
        return QVariant::fromValue(post.getImages());
    case Role::PostExternal:
    {
        auto external = post.getExternalView();
        return external ? QVariant::fromValue(*external) : QVariant();
    }
    case Role::PostRepostedByName:
    {
        const auto& repostedBy = post.getRepostedBy();

        if (!repostedBy)
            return {};

        const BasicProfile* profileChange = getProfileChange(repostedBy->getDid());
        return profileChange ? profileChange->getName() : repostedBy->getName();
    }
    case Role::PostRecord:
    {
        auto record = post.getRecordView();

        if (record)
        {
            const auto [visibility, warning] = mContentFilter.getVisibilityAndWarning(record->getLabels());
            record->setContentVisibility(visibility);
            record->setContentWarning(warning);
            record->setMutedReason(mMutedWords);
            return QVariant::fromValue(*record);
        }

        return QVariant();
    }
    case Role::PostRecordWithMedia:
    {
        auto recordWithMedia = post.getRecordWithMediaView();

        if (!recordWithMedia)
            return QVariant();

        auto& record = recordWithMedia->getRecord();
        const auto [visibility, warning] = mContentFilter.getVisibilityAndWarning(record.getLabels());
        record.setContentVisibility(visibility);
        record.setContentWarning(warning);
        record.setMutedReason(mMutedWords);
        return QVariant::fromValue(*recordWithMedia);
    }
    case Role::PostType:
        return post.getPostType();
    case Role::PostThreadType:
        return post.getThreadType();
    case Role::PostIsPlaceHolder:
        return post.isPlaceHolder();
    case Role::PostGapId:
        return post.getGapId();
    case Role::PostNotFound:
        return post.isNotFound();
    case Role::PostBlocked:
        return post.isBlocked();
    case Role::PostNotSupported:
        return post.isNotSupported();
    case Role::PostUnsupportedType:
        return post.getUnsupportedType();
    case Role::PostIsReply:
        return post.isReply();
    case Role::PostParentInThread:
        return post.isParentInThread();
    case Role::PostReplyToAuthor:
    {
        const auto author = post.getReplyToAuthor();

        if (!author)
            return {};

        const BasicProfile* profileChange = getProfileChange(author->getDid());
        return QVariant::fromValue(profileChange ? *profileChange : *author);
    }
    case Role::PostReplyRootCid:
        return post.getReplyRootCid();
    case Role::PostReplyRootUri:
        return post.getReplyRootUri();
    case Role::PostReplyCount:
        return post.getReplyCount() + (change ? change->mReplyCountDelta : 0);
    case Role::PostRepostCount:
        return post.getRepostCount() + (change ? change->mRepostCountDelta : 0);
    case Role::PostLikeCount:
        return post.getLikeCount() + (change ? change->mLikeCountDelta : 0);
    case Role::PostRepostUri:
        return change && change->mRepostUri ? *change->mRepostUri : post.getRepostUri();
    case Role::PostLikeUri:
        return change && change->mLikeUri ? *change->mLikeUri : post.getLikeUri();
    case Role::PostReplyDisabled:
        return post.isReplyDisabled();
    case Role::PostReplyRestriction:
        return post.getReplyRestriction();
    case Role::PostBookmarked:
        return mBookmarks.isBookmarked(post.getUri());
    case Role::PostBookmarkNotFound:
        return post.isBookmarkNotFound();
    case Role::PostLabels:
        return ContentFilter::getLabelTexts(post.getLabels());
    case Role::PostContentVisibility:
    {
        const auto [visibility, _] = mContentFilter.getVisibilityAndWarning(post.getLabels());
        return visibility;
    }
    case Role::PostContentWarning:
    {
        const auto [_, warning] = mContentFilter.getVisibilityAndWarning(post.getLabels());
        return warning;
    }
    case Role::PostMutedReason:
    {
        if (post.getAuthor().getViewer().isMuted())
            return QEnums::MUTED_POST_AUTHOR;

        if (mMutedWords.match(post))
            return QEnums::MUTED_POST_WORDS;

        return QEnums::MUTED_POST_NONE;
    }
    case Role::PostLocallyDeleted:
    {
        if (!change)
            return false;

        if (change->mPostDeleted)
            return true;

        if (!change->mRepostUri)
            return false;

        auto repostedBy = post.getRepostedBy();
        if (!repostedBy)
            return false;

        if (repostedBy->getDid() != mUserDid)
            return false;

        return post.getRepostUri() != *change->mRepostUri;
    }
    case Role::EndOfFeed:
        return post.isEndOfFeed();
    }

    qWarning() << "Uknown role requested:" << role;
    return {};
}

QHash<int, QByteArray> AbstractPostFeedModel::roleNames() const
{
    static const QHash<int, QByteArray> roles{
        { int(Role::Author), "author" },
        { int(Role::PostUri), "postUri" },
        { int(Role::PostCid), "postCid" },
        { int(Role::PostText), "postText" },
        { int(Role::PostPlainText), "postPlainText" },
        { int(Role::PostIndexedDateTime), "postIndexedDateTime" },
        { int(Role::PostRepostedByName), "postRepostedByName" },
        { int(Role::PostImages), "postImages" },
        { int(Role::PostExternal), "postExternal" },
        { int(Role::PostRecord), "postRecord" },
        { int(Role::PostRecordWithMedia), "postRecordWithMedia" },
        { int(Role::PostType), "postType" },
        { int(Role::PostThreadType), "postThreadType" },
        { int(Role::PostIsPlaceHolder), "postIsPlaceHolder" },
        { int(Role::PostGapId), "postGapId" },
        { int(Role::PostNotFound), "postNotFound" },
        { int(Role::PostBlocked), "postBlocked" },
        { int(Role::PostNotSupported), "postNotSupported" },
        { int(Role::PostUnsupportedType), "postUnsupportedType" },
        { int(Role::PostIsReply), "postIsReply" },
        { int(Role::PostParentInThread), "postParentInThread" },
        { int(Role::PostReplyToAuthor), "postReplyToAuthor" },
        { int(Role::PostReplyRootUri), "postReplyRootUri" },
        { int(Role::PostReplyRootCid), "postReplyRootCid" },
        { int(Role::PostReplyCount), "postReplyCount" },
        { int(Role::PostRepostCount), "postRepostCount" },
        { int(Role::PostLikeCount), "postLikeCount" },
        { int(Role::PostRepostUri), "postRepostUri" },
        { int(Role::PostLikeUri), "postLikeUri" },
        { int(Role::PostReplyDisabled), "postReplyDisabled" },
        { int(Role::PostReplyRestriction), "postReplyRestriction" },
        { int(Role::PostBookmarked), "postBookmarked" },
        { int(Role::PostBookmarkNotFound), "postBookmarkNotFound" },
        { int(Role::PostLabels), "postLabels" },
        { int(Role::PostContentVisibility), "postContentVisibility" },
        { int(Role::PostContentWarning), "postContentWarning" },
        { int(Role::PostMutedReason), "postMutedReason" },
        { int(Role::PostLocallyDeleted), "postLocallyDeleted" },
        { int(Role::EndOfFeed), "endOfFeed" }
    };

    return roles;
}

void AbstractPostFeedModel::postIndexTimestampChanged()
{
    changeData({ int(Role::PostIndexedDateTime) });
}

// For a change on a single post a single row change would be sufficient.
// However the model may have changed while waiting for a confirmation from the
// network about the change, so we don't know which row this CID is anymore.
// Also for reposts a CID may apply to multiple rows. Therefor all rows
// are marked as change. Performance seems not an issue.
void AbstractPostFeedModel::likeCountChanged()
{
    changeData({ int(Role::PostLikeCount) });
}

void AbstractPostFeedModel::likeUriChanged()
{
    changeData({ int(Role::PostLikeUri) });
}

void AbstractPostFeedModel::replyCountChanged()
{
    changeData({ int(Role::PostReplyCount) });
}

void AbstractPostFeedModel::repostCountChanged()
{
    changeData({ int(Role::PostRepostCount) });
}

void AbstractPostFeedModel::repostUriChanged()
{
    changeData({ int(Role::PostRepostUri), int(Role::PostLocallyDeleted) });
}

void AbstractPostFeedModel::postDeletedChanged()
{
    changeData({ int(Role::PostLocallyDeleted) });
}

void AbstractPostFeedModel::profileChanged()
{
    changeData({ int(Role::Author), int(Role::PostReplyToAuthor), int(Role::PostRepostedByName) });
}

void AbstractPostFeedModel::postBookmarkedChanged()
{
    changeData({ int(Role::PostBookmarked) });
}

void AbstractPostFeedModel::changeData(const QList<int>& roles)
{
    emit dataChanged(createIndex(0, 0), createIndex(mFeed.size() - 1, 0), roles);
}

}
