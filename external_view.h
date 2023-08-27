// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#pragma once
#include <atproto/lib/lexicon/app_bsky_embed.h>
#include <QtQmlIntegration>

namespace Skywalker
{

class ExternalView
{
    Q_GADGET
    Q_PROPERTY(QString uri READ getUri FINAL)
    Q_PROPERTY(QString title READ getTitle FINAL)
    Q_PROPERTY(QString description READ getDescription FINAL)
    Q_PROPERTY(QString thumbUrl READ getThumbUrl FINAL)
    QML_VALUE_TYPE(externalview)

public:
    using Ptr = std::unique_ptr<ExternalView>;

    ExternalView() = default;
    ExternalView(const ATProto::AppBskyEmbed::ExternalViewExternal* external) :
        mExternal(external)
    {}

    const QString& getUri() const { return mExternal->mUri; }
    const QString& getTitle() const { return mExternal->mTitle; }
    const QString& getDescription() const { return mExternal->mDescription; }
    const QString getThumbUrl() const { return mExternal->mThumb ? *mExternal->mThumb : QString(); }

private:
    const ATProto::AppBskyEmbed::ExternalViewExternal* mExternal = nullptr;
};

}

Q_DECLARE_METATYPE(Skywalker::ExternalView)
