// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#pragma once
#include "post_feed_model.h"
#include "post_thread_model.h"
#include "profile_store.h"
#include <atproto/lib/client.h>
#include <QObject>
#include <QTimer>
#include <QtQmlIntegration>

namespace Skywalker {

class Skywalker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(const PostFeedModel* timelineModel READ getTimelineModel CONSTANT FINAL)
    Q_PROPERTY(bool getTimelineInProgress READ isGetTimelineInProgress NOTIFY getTimeLineInProgressChanged FINAL)
    Q_PROPERTY(QString avatarUrl READ getAvatarUrl NOTIFY avatarUrlChanged FINAL)
    QML_ELEMENT
public:
    explicit Skywalker(QObject* parent = nullptr);
    ~Skywalker();

    Q_INVOKABLE void login(const QString user, QString password, const QString host);
    Q_INVOKABLE void resumeSession();
    Q_INVOKABLE void getUserProfileAndFollows();
    Q_INVOKABLE void syncTimeline(int maxPages = 40);
    Q_INVOKABLE void getTimeline(int limit, const QString& cursor = {});
    Q_INVOKABLE void getTimelinePrepend(int autoGapFill = 0);
    Q_INVOKABLE void getTimelineForGap(int gapId, int autoGapFill = 0);
    Q_INVOKABLE void getTimelineNextPage();
    Q_INVOKABLE void timelineMovementEnded(int firstVisibleIndex, int lastVisibleIndex);
    Q_INVOKABLE void getPostThread(const QString& uri);
    Q_INVOKABLE const PostThreadModel* getPostThreadModel(int id) const;
    Q_INVOKABLE void removePostThreadModel(int id);
    Q_INVOKABLE void pickPhoto();

    const PostFeedModel* getTimelineModel() const { return &mTimelineModel; }
    void setGetTimelineInProgress(bool inProgress);
    bool isGetTimelineInProgress() const { return mGetTimelineInProgress; }
    void setGetPostThreadInProgress(bool inProgress);
    const QString& getAvatarUrl() const { return mAvatarUrl; }
    void setAvatarUrl(const QString& avatarUrl);

signals:
    void loginOk();
    void loginFailed(QString error);
    void resumeSessionOk();
    void resumeSessionFailed();
    void timelineSyncOK(int index);
    void timelineSyncFailed();
    void getUserProfileOK();
    void getUserProfileFailed();
    void getTimeLineInProgressChanged();
    void sessionExpired(QString error);
    void statusMessage(QString msg, QEnums::StatusLevel level = QEnums::STATUS_LEVEL_INFO);
    void postThreadOk(int id, int postEntryIndex);
    void avatarUrlChanged();
    void photoPicked(QString filename);

private:
    std::optional<QString> makeOptionalCursor(const QString& cursor) const;
    void getUserProfileAndFollowsNextPage(const QString& cursor, int maxPages = 100);
    void signalGetUserProfileOk(const ATProto::AppBskyActor::ProfileView& user);
    void syncTimeline(QDateTime tillTimestamp, int maxPages = 40, const QString& cursor = {});
    void startRefreshTimer();
    void stopRefreshTimer();
    void refreshSession();
    void saveSession(const QString& host, const ATProto::ComATProtoServer::Session& session);
    bool getSession(QString& host, ATProto::ComATProtoServer::Session& session);
    void saveSyncTimestamp(int postIndex);
    QDateTime getSyncTimestamp() const;

    std::unique_ptr<ATProto::Client> mBsky;
    QString mAvatarUrl;
    QString mUserDid;
    ProfileStore mUserFollows;

    PostFeedModel mTimelineModel;
    bool mGetTimelineInProgress = false;
    bool mGetPostThreadInProgress = false;
    QTimer mRefreshTimer;

    std::unordered_map<int, PostThreadModel::Ptr> mPostThreadModels;
    int mNextPostThreadModelId = 1;
    QSettings mSettings;
};

}
