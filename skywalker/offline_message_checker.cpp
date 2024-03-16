// Copyright (C) 2024 Michel de Boer
// License: GPLv3
#include "offline_message_checker.h"
#include "notification.h"
#include "photo_picker.h"

#ifdef Q_OS_ANDROID
#include "android_utils.h"
#include "jni_utils.h"
#include <QJniObject>
#include <QtCore/private/qandroidextras_p.h>
#include <QTimer>
#endif

#if defined(Q_OS_ANDROID)
JNIEXPORT void JNICALL Java_com_gmail_mfnboer_NewMessageChecker_checkNewMessages(JNIEnv* env, jobject, jstring jSettingsFileName, jstring jLibDir)
{
    qSetMessagePattern("%{time HH:mm:ss.zzz} %{type} %{function}'%{line} %{message}");
    qDebug() << "CHECK NEW MESSAGES START";

    const char* settingsFileName = (*env).GetStringUTFChars(jSettingsFileName, nullptr);

    if (!settingsFileName)
    {
        qWarning() << "Settings file name missing";
        return;
    }

    const char* libDir = (*env).GetStringUTFChars(jLibDir, nullptr);

    if (!libDir)
    {
        qWarning() << "Library directory missing";
        return;
    }

    // This makes networking work???
    // Somehow Qt fails to find this class if we don't do this lookup.
    QJniEnvironment jniEnv;
    jclass javaClass = jniEnv.findClass("org/qtproject/qt/android/network/QtNetwork");

    if (!javaClass)
    {
        qWarning() << "Class loading failed";
        return;
    }

    std::unique_ptr<QCoreApplication> app;
    std::unique_ptr<QEventLoop> eventLoop;
    std::unique_ptr<Skywalker::OffLineMessageChecker> checker;

    if (!QCoreApplication::instance())
    {
        qDebug() << "Start core app";
        QCoreApplication::addLibraryPath(libDir);
        int argc = 1;
        const char* argv[] = {"Skywalker", NULL};
        app = std::make_unique<QCoreApplication>(argc, (char**)argv);
        qDebug() << "LIBS:" << app->libraryPaths();

        // Pass explicit file name as Qt does not know the app data path when running in
        // a background task.
        checker = std::make_unique<Skywalker::OffLineMessageChecker>(settingsFileName, app.get());
    }
    else
    {
        qDebug() << "App still running";
        eventLoop = std::make_unique<QEventLoop>();
        checker = std::make_unique<Skywalker::OffLineMessageChecker>(settingsFileName, eventLoop.get());
    }

    checker->check();

    (*env).ReleaseStringUTFChars(jSettingsFileName, settingsFileName);
    (*env).ReleaseStringUTFChars(jLibDir, libDir);
    qDebug() << "CHECK NEW MESSAGES END";
}
#endif

namespace Skywalker {

OffLineMessageChecker::OffLineMessageChecker(const QString& settingsFileName, QCoreApplication* backgroundApp) :
    mBackgroundApp(backgroundApp),
    mUserSettings(settingsFileName)
{
}

OffLineMessageChecker::OffLineMessageChecker(const QString& settingsFileName, QEventLoop* eventLoop) :
    mEventLoop(eventLoop),
    mUserSettings(settingsFileName)
{
}

void OffLineMessageChecker::startEventLoop()
{
    qDebug() << "Starting event loop";

    if (mBackgroundApp)
        mBackgroundApp->exec();
    else if (mEventLoop)
        mEventLoop->exec();
}

void OffLineMessageChecker::exit()
{
    qDebug() << "Exit";

    if (mBackgroundApp)
        mBackgroundApp->exit();
    else if (mEventLoop)
        mEventLoop->exit();
}

void OffLineMessageChecker::check()
{
    QObject guard;
    QTimer::singleShot(0, &guard, [this]{ resumeSession(); });
    startEventLoop();
}

void OffLineMessageChecker::createNotification(const BasicProfile& author, const QString& msg, const QDateTime& when)
{
    qDebug() << "Create notification:" << msg;

#if defined(Q_OS_ANDROID)
    QJniEnvironment env;
    QJniObject jTitle = QJniObject::fromString(author.getName());
    QJniObject jMsg = QJniObject::fromString(msg);
    jlong jWhen = when.toMSecsSinceEpoch();

    jbyteArray jAvatar = nullptr;
    const auto avatarUrl = author.getAvatarUrl();

    if (!avatarUrl.isEmpty() && mAvatars.count(avatarUrl))
    {
        const QByteArray& jpg = mAvatars[avatarUrl];
        jAvatar = env->NewByteArray(jpg.size());
        env->SetByteArrayRegion(jAvatar, 0, jpg.size(), (jbyte*)jpg.data());
    }
    else
    {
        jAvatar = env->NewByteArray(0);
    }

    auto [javaClass, methodId] = JniUtils::getClassAndMethod(
        env,
        "com/gmail/mfnboer/NewMessageNotifier",
        "createNotification",
        "(Ljava/lang/String;Ljava/lang/String;J[B)V");

    if (!javaClass || !methodId)
        return;

    QJniObject::callStaticMethod<void>(
        javaClass,
        methodId,
        jTitle.object<jstring>(),
        jMsg.object<jstring>(),
        jWhen,
        jAvatar);
#else
    Q_UNUSED(title);
    Q_UNUSED(msg);
#endif
}

bool OffLineMessageChecker::getSession(QString& host, ATProto::ComATProtoServer::Session& session)
{
    const QString did = mUserSettings.getActiveUserDid();

    if (did.isEmpty())
        return false;

    session = mUserSettings.getSession(did);

    if (session.mAccessJwt.isEmpty() || session.mRefreshJwt.isEmpty())
        return false;

    host = mUserSettings.getHost(did);

    if (host.isEmpty())
        return false;

    return true;
}

void OffLineMessageChecker::saveSession(const ATProto::ComATProtoServer::Session& session)
{
    mUserSettings.saveSession(session);
}

void OffLineMessageChecker::resumeSession()
{
    qDebug() << "Resume session";
    QString host;
    ATProto::ComATProtoServer::Session session;

    if (!getSession(host, session))
    {
        qWarning() << "No saved session";
        exit();
        return;
    }

    mUserDid = session.mDid;
    auto xrpc = std::make_unique<Xrpc::Client>(host);
    mBsky = std::make_unique<ATProto::Client>(std::move(xrpc));

    mBsky->resumeSession(session,
        [this] {
            qDebug() << "Session resumed";
            saveSession(*mBsky->getSession());
            refreshSession();
        },
        [this](const QString& error, const QString& msg){
            qWarning() << "Session could not be resumed:" << error << " - " << msg;

            if (error == "ExpiredToken")
                login();
            else
                exit();
        });
}

void OffLineMessageChecker::refreshSession()
{
    Q_ASSERT(mBsky);
    qDebug() << "Refresh session";

    const auto* session = mBsky->getSession();
    if (!session)
    {
        qWarning() << "No session to refresh.";
        exit();
        return;
    }

    mBsky->refreshSession(
        [this]{
            qDebug() << "Session refreshed";
            saveSession(*mBsky->getSession());
            checkUnreadNotificationCount();
        },
        [this](const QString& error, const QString& msg){
            qWarning() << "Session could not be refreshed:" << error << " - " << msg;
            exit();
        });
}

void OffLineMessageChecker::login()
{
    const auto host = mUserSettings.getHost(mUserDid);
    const auto password = mUserSettings.getPassword(mUserDid);

    if (host.isEmpty() || password.isEmpty())
    {
        qWarning() << "Missing host or password to login";
        exit();
        return;
    }

    qDebug() << "Login:" << mUserDid << "host:" << host;
    auto xrpc = std::make_unique<Xrpc::Client>(host);
    mBsky = std::make_unique<ATProto::Client>(std::move(xrpc));

    mBsky->createSession(mUserDid, password,
        [this]{
            qDebug() << "Login" << mUserDid << "succeeded";
            const auto* session = mBsky->getSession();
            saveSession(*session);
            checkUnreadNotificationCount();
        },
        [this](const QString& error, const QString& msg){
            qWarning() << "Login" << mUserDid << "failed:" << error << " - " << msg;
            exit();
        });
}

void OffLineMessageChecker::checkUnreadNotificationCount()
{
    const int prevUnread = mUserSettings.getOfflineUnread(mUserDid);
    qDebug() << "Check unread notification count, last unread:" << prevUnread;

    mBsky->getUnreadNotificationCount({},
        [this, prevUnread](int unread){
            qDebug() << "Unread notification count:" << unread;
            int newCount = unread - prevUnread;

            if (newCount < 0)
            {
                qDebug() << "Unread notification count has been reset by another client";
                newCount = unread;
                mUserSettings.setOfflineUnread(mUserDid, 0);
                mUserSettings.sync();
            }

            if (newCount == 0)
            {
                qDebug() << "No new messages";
                exit();
                return;
            }

            getNotifications(newCount);
        },
        [this](const QString& error, const QString& msg){
            qWarning() << "Failed to get unread notification count:" << error << " - " << msg;
            exit();
        });
}

void OffLineMessageChecker::getNotifications(int toRead)
{
    qDebug() << "Get notifications:" << toRead;
    const int limit = std::min(toRead, 100); // 100 is max that can be retreived in 1 request

    mBsky->listNotifications(limit, {}, {},
        [this, toRead](auto notifications){
            mNotificationsList = std::move(notifications);
            getAvatars();
            const int prevUnread = mUserSettings.getOfflineUnread(mUserDid);
            mUserSettings.setOfflineUnread(mUserDid, prevUnread + toRead);
            mUserSettings.sync();
        },
        [this](const QString& error, const QString& msg){
            qDebug() << "getNotifications FAILED:" << error << " - " << msg;
            exit();
        },
        false);
}

void OffLineMessageChecker::getAvatars()
{
    std::set<QString> avatarUrls;

    for (const auto& notification : mNotificationsList->mNotifications)
    {
        if (notification->mAuthor->mAvatar)
            avatarUrls.insert(*notification->mAuthor->mAvatar);
    }

    const QStringList urls(avatarUrls.begin(), avatarUrls.end());
    getAvatars(urls);
}

void OffLineMessageChecker::getAvatars(const QStringList& urls)
{
    if (urls.isEmpty())
    {
        createNotifications();
        return;
    }

    const QString url = urls.front();
    const auto remainingUrls = urls.mid(1);
    qDebug() << "Get avatar:" << url;

    const bool validUrl = mImageReader.getImage(url,
        [this, url, remainingUrls](QImage image){
            QByteArray jpgBlob;
            PhotoPicker::createBlob(jpgBlob, image);

            if (!jpgBlob.isNull())
                mAvatars[url] = jpgBlob;
            else
                qWarning() << "Could not convert avatar to JPG:" << url;

            getAvatars(remainingUrls);
        },
        [this, remainingUrls](const QString& error){
            qWarning() << "Failed to get avatar:" << error;
            getAvatars(remainingUrls);
        });

    if (!validUrl)
        getAvatars(remainingUrls);
}

void OffLineMessageChecker::createNotifications()
{
    qDebug() << "Create notifications:" << mNotificationsList->mNotifications.size();

    for (const auto& notification : mNotificationsList->mNotifications)
        createNotification(notification.get());

    exit();
}

void OffLineMessageChecker::createNotification(const ATProto::AppBskyNotification::Notification* protoNotification)
{
    const Notification notification(protoNotification);
    const PostRecord postRecord = notification.getPostRecord();
    const QString postText = !postRecord.isNull() ? postRecord.getText() : "";
    QString msg;

    // TODO: postText can be empty if there is only an image.

    switch (notification.getReason())
    {
    case ATProto::AppBskyNotification::NotificationReason::LIKE:
        msg = QObject::tr("Liked your post\n") + postText; // TODO: post is not in record
        break;
    case ATProto::AppBskyNotification::NotificationReason::REPOST:
        msg = QObject::tr("Reposted your post\n") + postText; // TODO: post is not in record
        break;
    case ATProto::AppBskyNotification::NotificationReason::FOLLOW:
        msg = QObject::tr("Follows you");
        break;
    case ATProto::AppBskyNotification::NotificationReason::MENTION:
        msg = postText;
        break;
    case ATProto::AppBskyNotification::NotificationReason::REPLY:
        msg = postText;
        break;
    case ATProto::AppBskyNotification::NotificationReason::QUOTE:
        msg = postText;
        break;
    case ATProto::AppBskyNotification::NotificationReason::INVITE_CODE_USED:
    case ATProto::AppBskyNotification::NotificationReason::UNKNOWN:
        return;
    }

    QDateTime when = notification.getTimestamp();

    if (when.isNull())
        when = QDateTime::currentDateTimeUtc();

    createNotification(notification.getAuthor(), msg, when);
}

bool OffLineMessageChecker::checkNoticationPermission()
{
#if defined(Q_OS_ANDROID)
    static constexpr char const* POST_NOTIFICATIONS = "android.permission.POST_NOTIFICATIONS";

    if (!AndroidUtils::checkPermission(POST_NOTIFICATIONS))
    {
        qDebug() << "No permission:" << POST_NOTIFICATIONS;
        return false;
    }

    return true;
#else
    return false;
#endif
}

void OffLineMessageChecker::start()
{
#if defined(Q_OS_ANDROID)
    if (!checkNoticationPermission())
        return;

    QJniObject::callStaticMethod<void>("com/gmail/mfnboer/NewMessageChecker", "startChecker");
#endif
}

void OffLineMessageChecker::createNotificationChannel()
{
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>("com/gmail/mfnboer/NewMessageNotifier", "createNotificationChannel");
#endif
}

}
