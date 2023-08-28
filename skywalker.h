// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#pragma once
#include "post_feed_model.h"
#include <atproto/lib/client.h>
#include <QObject>
#include <QtQmlIntegration>

namespace Skywalker {

class Skywalker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(const PostFeedModel* timelineModel READ getTimelineModel NOTIFY timelineModelChanged)
    QML_ELEMENT
public:
    explicit Skywalker(QObject* parent = nullptr);

    Q_INVOKABLE void login(const QString user, QString password, const QString host);
    Q_INVOKABLE void getTimeline();

    const PostFeedModel* getTimelineModel() const { return &mTimelineModel; }

signals:
    void loginOk();
    void loginFailed(QString error);
    void timelineModelChanged();

private:
    std::unique_ptr<ATProto::Client> mBsky;
    PostFeedModel mTimelineModel;
};

}