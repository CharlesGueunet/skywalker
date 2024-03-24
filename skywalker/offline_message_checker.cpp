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

using namespace std::chrono_literals;

namespace {
constexpr char const* CHANNEL_POST = "CHANNEL_POST";
constexpr char const* CHANNEL_LIKE = "CHANNEL_LIKE";
constexpr char const* CHANNEL_REPOST = "CHANNEL_REPOST";
constexpr char const* CHANNEL_FOLLOW = "CHANNEL_FOLLOW";

constexpr int EXIT_OK = 0;
constexpr int EXIT_RETRY = -1;
}

#if defined(Q_OS_ANDROID)
JNIEXPORT int JNICALL Java_com_gmail_mfnboer_NewMessageChecker_checkNewMessages(JNIEnv* env, jobject, jstring jSettingsFileName, jstring jLibDir)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    qSetMessagePattern("%{time HH:mm:ss.zzz} %{type} %{function}'%{line} %{message}");
    qDebug() << "CHECK NEW MESSAGES START";

    const char* settingsFileName = (*env).GetStringUTFChars(jSettingsFileName, nullptr);

    if (!settingsFileName)
    {
        qWarning() << "Settings file name missing";
        return EXIT_OK;
    }

    const char* libDir = (*env).GetStringUTFChars(jLibDir, nullptr);

    if (!libDir)
    {
        qWarning() << "Library directory missing";
        return EXIT_OK;
    }

    // This makes networking work???
    // Somehow Qt fails to find this class if we don't do this lookup.
    QJniEnvironment jniEnv;
    jclass javaClass = jniEnv.findClass("org/qtproject/qt/android/network/QtNetwork");

    if (!javaClass)
    {
        qWarning() << "Class loading failed";
        return EXIT_OK;
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

    int exitCode = checker->check();

    (*env).ReleaseStringUTFChars(jSettingsFileName, settingsFileName);
    (*env).ReleaseStringUTFChars(jLibDir, libDir);
    qDebug() << "CHECK NEW MESSAGES END:" << exitCode;

    return exitCode;
}
#endif

namespace Skywalker {

const std::vector<NotificationChannel> OffLineMessageChecker::NOTIFCATION_CHANNELS = {
    {
        CHANNEL_POST,
        QObject::tr("Posts"),
        QObject::tr("Replies to your posts, mentions and quoted posts")
    },
    {
        CHANNEL_LIKE,
        QObject::tr("Likes"),
        QObject::tr("Likes on your posts")
    },
    {
        CHANNEL_REPOST,
        QObject::tr("Reposts"),
        QObject::tr("Reposts of your posts")
    },
    {
        CHANNEL_FOLLOW,
        QObject::tr("Follows"),
        QObject::tr("New followers")
    }
};

OffLineMessageChecker::OffLineMessageChecker(const QString& settingsFileName, QCoreApplication* backgroundApp) :
    mBackgroundApp(backgroundApp),
    mUserSettings(settingsFileName),
    mContentFilter(mUserPreferences),
    mNotificationListModel(mContentFilter, mBookmarks, mMutedWords)
{
    mNotificationListModel.enableRetrieveNotificationPosts(false);
}

OffLineMessageChecker::OffLineMessageChecker(const QString& settingsFileName, QEventLoop* eventLoop) :
    mEventLoop(eventLoop),
    mUserSettings(settingsFileName),
    mContentFilter(mUserPreferences),
    mNotificationListModel(mContentFilter, mBookmarks, mMutedWords)
{
    mNotificationListModel.enableRetrieveNotificationPosts(false);
}

int OffLineMessageChecker::startEventLoop()
{
    qDebug() << "Starting event loop";

    if (mBackgroundApp)
        return mBackgroundApp->exec();
    else if (mEventLoop)
        return mEventLoop->exec();

    return EXIT_OK;
}

void OffLineMessageChecker::exit(int exitCode)
{
    qDebug() << "Exit";

    if (mBackgroundApp)
        mBackgroundApp->exit(exitCode);
    else if (mEventLoop)
        mEventLoop->exit(exitCode);
}

int OffLineMessageChecker::check()
{
    const auto timestamp = mUserSettings.getOfflineMessageCheckTimestamp();
    qDebug() << "Previous check:" << timestamp;

    if (!timestamp.isNull())
    {
        const auto now = QDateTime::currentDateTime();

        if (now - timestamp < 2min)
        {
            qDebug() << "Retry later";
            return EXIT_RETRY;
        }
    }

    QObject guard;
    QTimer::singleShot(0, &guard, [this]{ resumeSession(); });
    startEventLoop();
    mUserSettings.setOfflineMessageCheckTimestamp(QDateTime::currentDateTime());
    return EXIT_OK;
}

void OffLineMessageChecker::createNotification(const QString channelId, const BasicProfile& author, const QString& msg, const QDateTime& when, IconType iconType)
{
    qDebug() << "Create notification:" << msg;

#if defined(Q_OS_ANDROID)
    QJniEnvironment env;
    QJniObject jChannelId = QJniObject::fromString(channelId);
    jint jNotificationId = mUserSettings.getNextNotificationId();
    QJniObject jTitle = QJniObject::fromString(author.getName());
    QJniObject jMsg = QJniObject::fromString(msg);
    jlong jWhen = when.toMSecsSinceEpoch();
    jint jIconType = (int)iconType;

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
        "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;JI[B)V");

    if (!javaClass || !methodId)
        return;

    QJniObject::callStaticMethod<void>(
        javaClass,
        methodId,
        jChannelId.object<jstring>(),
        jNotificationId,
        jTitle.object<jstring>(),
        jMsg.object<jstring>(),
        jWhen,
        jIconType,
        jAvatar);
#else
    Q_UNUSED(channelId);
    Q_UNUSED(author);
    Q_UNUSED(msg);
    Q_UNUSED(when);
    Q_UNUSED(iconType);
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
        exit(EXIT_OK);
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
                exit(EXIT_OK);
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
        exit(EXIT_OK);
        return;
    }

    mBsky->refreshSession(
        [this]{
            qDebug() << "Session refreshed";
            saveSession(*mBsky->getSession());
            getUserPreferences();
        },
        [this](const QString& error, const QString& msg){
            qWarning() << "Session could not be refreshed:" << error << " - " << msg;
            exit(EXIT_OK);
        });
}

void OffLineMessageChecker::login()
{
    const auto host = mUserSettings.getHost(mUserDid);
    const auto password = mUserSettings.getPassword(mUserDid);

    if (host.isEmpty() || password.isEmpty())
    {
        qWarning() << "Missing host or password to login";
        exit(EXIT_OK);
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
            getUserPreferences();
        },
        [this](const QString& error, const QString& msg){
            qWarning() << "Login" << mUserDid << "failed:" << error << " - " << msg;
            exit(EXIT_OK);
        });
}

void OffLineMessageChecker::getUserPreferences()
{
    qDebug() << "Get user preferences";

    mBsky->getPreferences(
        [this](auto prefs){
            mUserPreferences = prefs;
            mMutedWords.load(mUserPreferences);
            checkUnreadNotificationCount();
        },
        [this](const QString& error, const QString& msg){
            qWarning() << error << " - " << msg;
            exit(EXIT_OK);
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
                exit(EXIT_OK);
                return;
            }

            getNotifications(newCount);
        },
        [this](const QString& error, const QString& msg){
            qWarning() << "Failed to get unread notification count:" << error << " - " << msg;
            exit(EXIT_OK);
        });
}

void OffLineMessageChecker::getNotifications(int toRead)
{
    qDebug() << "Get notifications:" << toRead;
    const int limit = std::min(toRead, 100); // 100 is max that can be retreived in 1 request

    mBsky->listNotifications(limit, {}, {},
        [this, toRead](auto notifications){
            const bool added = mNotificationListModel.addNotifications(std::move(notifications), *mBsky, false,
                [this]{ getAvatars(); });

            const int prevUnread = mUserSettings.getOfflineUnread(mUserDid);
            mUserSettings.setOfflineUnread(mUserDid, prevUnread + toRead);
            mUserSettings.sync();

            if (!added)
                exit(EXIT_OK);
        },
        [this](const QString& error, const QString& msg){
            qDebug() << "getNotifications FAILED:" << error << " - " << msg;
            exit(EXIT_OK);
        },
        false);
}

void OffLineMessageChecker::getAvatars()
{
    std::set<QString> avatarUrls;
    const auto& notifications = mNotificationListModel.getNotifications();

    for (const auto& notification : notifications)
    {
        const QString url = notification.getAuthor().getAvatarUrl();

        if (!url.isEmpty())
            avatarUrls.insert(url);
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
    const auto& notifications = mNotificationListModel.getNotifications();
    qDebug() << "Create notifications:" << notifications.size();

    for (const auto& notification : notifications)
        createNotification(notification);

    exit(EXIT_OK);
}

void OffLineMessageChecker::createNotification(const Notification& notification)
{
    const PostCache& reasonPostCache = mNotificationListModel.getReasonPostCache();
    const PostRecord postRecord = notification.getPostRecord();

    // NOTE: postText can be empty if there is only an image.
    QString msg = !postRecord.isNull() ? postRecord.getFormattedText() : "";
    QString channelId = CHANNEL_POST;
    IconType iconType = IconType::CHAT;

    switch (notification.getReason())
    {
    case ATProto::AppBskyNotification::NotificationReason::LIKE:
    {
        channelId = CHANNEL_LIKE;
        iconType = IconType::LIKE;
        msg = QObject::tr("<b>Liked your post</b>");
        const Post post = notification.getReasonPost(reasonPostCache);
        const auto reasonPostText = post.getFormattedText();

        if (!reasonPostText.isEmpty())
            msg.append("<br>" + reasonPostText);

        break;
    }
    case ATProto::AppBskyNotification::NotificationReason::REPOST:
    {
        channelId = CHANNEL_REPOST;
        iconType = IconType::REPOST;
        msg = QObject::tr("<b>Reposted your post</b>");
        const Post post = notification.getReasonPost(reasonPostCache);
        const auto reasonPostText = post.getFormattedText();

        if (!reasonPostText.isEmpty())
            msg.append("<br>" + reasonPostText);

        break;
    }
    case ATProto::AppBskyNotification::NotificationReason::FOLLOW:
        channelId = CHANNEL_FOLLOW;
        iconType = IconType::FOLLOW;
        msg = QObject::tr("<b>Follows you</b>");
        break;
    case ATProto::AppBskyNotification::NotificationReason::MENTION:
        iconType = IconType::MENTION;
        break;
    case ATProto::AppBskyNotification::NotificationReason::REPLY:
        break;
    case ATProto::AppBskyNotification::NotificationReason::QUOTE:
        break;
    case ATProto::AppBskyNotification::NotificationReason::INVITE_CODE_USED:
    case ATProto::AppBskyNotification::NotificationReason::UNKNOWN:
        return;
    }

    QDateTime when = notification.getTimestamp();

    if (when.isNull())
        when = QDateTime::currentDateTimeUtc();

    createNotification(channelId, notification.getAuthor(), msg, when, iconType);
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

void OffLineMessageChecker::start(bool wifiOnly)
{
#if defined(Q_OS_ANDROID)
    if (!checkNoticationPermission())
        return;

    jboolean jWifiOnly = wifiOnly;

    QJniObject::callStaticMethod<void>(
        "com/gmail/mfnboer/NewMessageChecker",
        "startChecker",
        "(Z)V",
        jWifiOnly);
#endif
}

void OffLineMessageChecker::createNotificationChannels()
{
#if defined(Q_OS_ANDROID)
    QJniEnvironment env;

    for (const auto& channel : NOTIFCATION_CHANNELS)
    {
        QJniObject jId = QJniObject::fromString(channel.mId);
        QJniObject jName = QJniObject::fromString(channel.mName);
        QJniObject jDescription = QJniObject::fromString(channel.mDescription);

        QJniObject::callStaticMethod<void>(
            "com/gmail/mfnboer/NewMessageNotifier",
            "createNotificationChannel",
            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
            jId.object<jstring>(),
            jName.object<jstring>(),
            jDescription.object<jstring>());
    }
#endif
}

}
