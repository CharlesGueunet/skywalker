// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#include "post.h"

namespace Skywalker {

Post::Post(const ATProto::AppBskyFeed::FeedViewPost* feedViewPost) :
    mFeedViewPost(feedViewPost)
{
    Q_ASSERT(mFeedViewPost);
}

QString Post::getText() const
{
    const auto& post = mFeedViewPost->mPost;
    if (post->mRecordType == ATProto::RecordType::APP_BSKY_FEED_POST)
        return std::get<ATProto::AppBskyFeed::Record::Post::Ptr>(post->mRecord)->mText;

    return {};
}

BasicProfile Post::getAuthor() const
{
    return BasicProfile(mFeedViewPost->mPost->mAuthor.get());
}

QDateTime Post::getCreatedAt() const
{
    const auto& post = mFeedViewPost->mPost;
    if (post->mRecordType == ATProto::RecordType::APP_BSKY_FEED_POST)
        return std::get<ATProto::AppBskyFeed::Record::Post::Ptr>(post->mRecord)->mCreatedAt;

    return {};
}

std::optional<BasicProfile> Post::getRepostedBy() const
{
    if (!mFeedViewPost->mReason)
        return {};

    return BasicProfile(mFeedViewPost->mReason->mBy.get());
}

std::vector<ImageView::Ptr> Post::getImages() const
{
    const auto& post = mFeedViewPost->mPost;

    if (!post->mEmbed || post->mEmbed->mType != ATProto::AppBskyEmbed::EmbedType::IMAGES_VIEW)
        return {};

    const auto& imagesView = std::get<ATProto::AppBskyEmbed::ImagesView::Ptr>(post->mEmbed->mEmbed);
    std::vector<ImageView::Ptr> images;

    for (const auto& img : imagesView->mImages)
        images.push_back(std::make_unique<ImageView>(img.get()));

    return images;
}

ExternalView::Ptr Post::getExternalView() const
{
    const auto& post = mFeedViewPost->mPost;

    if (!post->mEmbed || post->mEmbed->mType != ATProto::AppBskyEmbed::EmbedType::EXTERNAL_VIEW)
        return {};

    const auto& external = std::get<ATProto::AppBskyEmbed::ExternalView::Ptr>(post->mEmbed->mEmbed)->mExternal;
    return std::make_unique<ExternalView>(external.get());
}

RecordView::Ptr Post::getRecordView() const
{
    const auto& post = mFeedViewPost->mPost;

    if (!post->mEmbed || post->mEmbed->mType != ATProto::AppBskyEmbed::EmbedType::RECORD_VIEW)
        return {};

    const auto& recordView = std::get<ATProto::AppBskyEmbed::RecordView::Ptr>(post->mEmbed->mEmbed);
    return std::make_unique<RecordView>(*recordView);
}

RecordWithMediaView::Ptr Post::getRecordWithMediaView() const
{
    const auto& post = mFeedViewPost->mPost;

    if (!post->mEmbed || post->mEmbed->mType != ATProto::AppBskyEmbed::EmbedType::RECORD_WITH_MEDIA_VIEW)
        return {};

    const auto& recordView = std::get<ATProto::AppBskyEmbed::RecordWithMediaView::Ptr>(post->mEmbed->mEmbed);
    return std::make_unique<RecordWithMediaView>(recordView.get());
}

}