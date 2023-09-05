// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#include "skywalker.h"
#include <QSettings>

namespace Skywalker {

using namespace std::chrono_literals;

static constexpr auto SESSION_REFRESH_INTERVAL = 901s;
static constexpr int TIMELINE_ADD_PAGE_SIZE = 50;
static constexpr int TIMELINE_PREPEND_PAGE_SIZE = 20;

Skywalker::Skywalker(QObject* parent) :
    QObject(parent)
{
    connect(&mRefreshTimer, &QTimer::timeout, this, [this]{ refreshSession(); });
}

void Skywalker::login(const QString user, QString password, const QString host)
{
    qDebug() << "Login:" << user << "host:" << host;
    auto xrpc = std::make_unique<Xrpc::Client>(host);
    mBsky = std::make_unique<ATProto::Client>(std::move(xrpc));
    mBsky->createSession(user, password,
        [this, user, host]{
            qDebug() << "Login" << user << "succeded";
            SaveSession(host, *mBsky->getSession());
            emit loginOk();
            startRefreshTimer();
        },
        [this, user](const QString& error){
            qDebug() << "Login" << user << "failed:" << error;
            emit loginFailed(error);
        });
}

void Skywalker::resumeSession()
{
    qDebug() << "Resume session";
    QString host;
    ATProto::ComATProtoServer::Session session;

    if (!GetSession(host, session))
    {
        qWarning() << "No saved session";
        emit resumeSessionFailed();
        return;
    }

    auto xrpc = std::make_unique<Xrpc::Client>(host);
    mBsky = std::make_unique<ATProto::Client>(std::move(xrpc));

    mBsky->resumeSession(session,
        [this, host] {
            qDebug() << "Session resumed";
            SaveSession(host, *mBsky->getSession());
            emit resumeSessionOk();
            refreshSession();
            startRefreshTimer();
        },
        [this](const QString& error){
            qDebug() << "Session could not be resumed:" << error;
            emit resumeSessionFailed();
        });
}

void Skywalker::startRefreshTimer()
{
    qDebug() << "Refresh timer started";
    mRefreshTimer.start(SESSION_REFRESH_INTERVAL);
}

void Skywalker::stopRefreshTimer()
{
    qDebug() << "Refresh timer stopped";
    mRefreshTimer.stop();
}

void Skywalker::refreshSession()
{
    Q_ASSERT(mBsky);
    qDebug() << "Refresh session";

    const auto* session = mBsky->getSession();
    if (!session)
    {
        qWarning() << "No session to refresh.";
        stopRefreshTimer();
        return;
    }

    mBsky->refreshSession(*session,
        [this] {
            qDebug() << "Session refreshed";
            SaveSession(mBsky->getHost(), *mBsky->getSession());
        },
        [this](const QString& error){
            qDebug() << "Session could not be refreshed:" << error;
            emit sessionExpired(error);
            stopRefreshTimer();
        });
}

void Skywalker::getTimeline(int limit, const QString& cursor)
{
    Q_ASSERT(mBsky);
    qDebug() << "Get timeline:" << cursor;

    if (mGetTimelineInProgress)
    {
        qDebug() << "Get timeline still in progress";
        return;
    }

    std::optional<QString> cur;
    if (!cursor.isEmpty())
        cur = cursor;

    setGetTimelineInProgress(true);
    mBsky->getTimeline(limit, cur,
       [this, cursor](auto feed){
            if (cursor.isEmpty())
                mTimelineModel.setFeed(std::move(feed));
            else
                mTimelineModel.addFeed(std::move(feed));

            setGetTimelineInProgress(false);
       },
       [this](const QString& error){
            qDebug() << "getTimeline FAILED:" << error;
            setGetTimelineInProgress(false);
        }
    );
    // TODO: show error in GUI
}

void Skywalker::getTimelinePrepend(int autoGapFill)
{
    Q_ASSERT(mBsky);
    qDebug() << "Get timeline prepend";

    if (mGetTimelineInProgress)
    {
        qDebug() << "Get timeline still in progress";
        return;
    }

    if (mTimelineModel.rowCount() >= PostFeedModel::MAX_TIMELINE_SIZE)
    {
        qWarning() << "Timeline is full:" << mTimelineModel.rowCount();
        return;
    }

    setGetTimelineInProgress(true);
    mBsky->getTimeline(TIMELINE_PREPEND_PAGE_SIZE, {},
        [this, autoGapFill](auto feed){
            const int gapId = mTimelineModel.prependFeed(std::move(feed));
            setGetTimelineInProgress(false);

            if (gapId > 0)
            {
                if (autoGapFill > 0)
                    getTimelineForGap(gapId, autoGapFill - 1);
                else
                    qWarning() << "Gap created, no auto gap fill";
            }
        },
        [this](const QString& error){
            qDebug() << "getTimeline FAILED:" << error;
            setGetTimelineInProgress(false);
        }
        );
    // TODO: show error in GUI
}

void Skywalker::getTimelineForGap(int gapId, int autoGapFill)
{
    Q_ASSERT(mBsky);
    qDebug() << "Get timeline for gap:" << gapId;

    if (mGetTimelineInProgress)
    {
        qDebug() << "Get timeline still in progress";
        return;
    }

    const Post* post = mTimelineModel.getGapPlaceHolder(gapId);
    if (!post || !post->isPlaceHolder())
    {
        qWarning() << "NO GAP:" << gapId;
        return;
    }

    std::optional<QString> cur = post->getGapCursor();
    if (!cur || cur->isEmpty())
    {
        qWarning() << "NO CURSOR FOR GAP:" << gapId;
        return;
    }

    qDebug() << "Set gap cursor:" << *cur;

    setGetTimelineInProgress(true);
    mBsky->getTimeline(TIMELINE_ADD_PAGE_SIZE, cur,
        [this, gapId, autoGapFill](auto feed){
            const int newGapId = mTimelineModel.gapFillFeed(std::move(feed), gapId);
            setGetTimelineInProgress(false);

            if (newGapId > 0)
            {
                if (autoGapFill > 0)
                    getTimelineForGap(newGapId, autoGapFill - 1);
                else
                    qWarning() << "Gap created, no auto gap fill";
            }
        },
        [this](const QString& error){
            qDebug() << "getTimelineForGap FAILED:" << error;
            setGetTimelineInProgress(false);
        }
        );
    // TODO: show error in GUI
}

void Skywalker::getTimelineNextPage()
{
    const QString& cursor = mTimelineModel.getLastCursor();
    if (cursor.isEmpty())
    {
        qDebug() << "Last page reached, no more cursor";
        return;
    }

    if (mTimelineModel.rowCount() >= PostFeedModel::MAX_TIMELINE_SIZE)
        mTimelineModel.removeHeadPosts(TIMELINE_ADD_PAGE_SIZE);

    getTimeline(TIMELINE_ADD_PAGE_SIZE, cursor);
}

void Skywalker::setGetTimelineInProgress(bool inProgress)
{
    mGetTimelineInProgress = inProgress;
    emit getTimeLineInProgressChanged();
}

// NOTE: indices can be -1 if the UI cannot determine the index
void Skywalker::timelineMovementEnded(int firstVisibleIndex, int lastVisibleIndex)
{
    if (lastVisibleIndex > -1 && mTimelineModel.rowCount() - lastVisibleIndex > 2 * TIMELINE_ADD_PAGE_SIZE)
        mTimelineModel.removeTailPosts(TIMELINE_ADD_PAGE_SIZE);

    if (lastVisibleIndex > mTimelineModel.rowCount() - 5 && !mGetTimelineInProgress)
        getTimelineNextPage();
}

void Skywalker::SaveSession(const QString& host, const ATProto::ComATProtoServer::Session& session)
{
    QSettings settings;
    settings.setValue("host", host);
    settings.setValue("did", session.mDid);
    settings.setValue("access", session.mAccessJwt);
    settings.setValue("refresh", session.mRefreshJwt);
}

bool Skywalker::GetSession(QString& host, ATProto::ComATProtoServer::Session& session)
{
    QSettings settings;
    host = settings.value("host").toString();
    session.mDid = settings.value("did").toString();
    session.mAccessJwt = settings.value("access").toString();
    session.mRefreshJwt = settings.value("refresh").toString();

    return !(host.isEmpty() || session.mDid.isEmpty() || session.mAccessJwt.isEmpty() || session.mRefreshJwt.isEmpty());
}

}
