// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#include "user_settings.h"

namespace Skywalker {

static constexpr char const* KEY_ALIAS_PASSWORD = "SkywalkerPass";
static constexpr char const* KEY_ALIAS_ACCESS_TOKEN = "SkywalkerAccess";
static constexpr char const* KEY_ALIAS_REFRESH_TOKEN = "SkywalkerRefresh";

UserSettings::UserSettings(QObject* parent) :
    QObject(parent)
{
    mEncryption.init(KEY_ALIAS_PASSWORD);
    mEncryption.init(KEY_ALIAS_ACCESS_TOKEN);
    mEncryption.init(KEY_ALIAS_REFRESH_TOKEN);
}

QString UserSettings::key(const QString& did, const QString& subkey) const
{
    return QString("%1/%2").arg(did, subkey);
}

QList<BasicProfile> UserSettings::getUserList() const
{
    const auto didList = getUserDidList();
    QList<BasicProfile> userList;

    for (const auto& did : didList)
    {
        BasicProfile profile(
            did,
            getHandle(did),
            getDisplayName(did),
            getAvatar(did));

        userList.append(profile);
    }

    return userList;
}

QStringList UserSettings::getUserDidList() const
{
    return mSettings.value("users").toStringList();
}

void UserSettings::setActiveUser(const QString& did)
{
    mSettings.setValue("activeUser", did);
}

QString UserSettings::getActiveUser() const
{
    return mSettings.value("activeUser").toString();
}

void UserSettings::addUser(const QString& did, const QString& host)
{
    auto users = getUserDidList();
    users.append(did);
    users.sort();
    mSettings.setValue("users", users);
    mSettings.setValue(key(did, "host"), host);
}

void UserSettings::removeUser(const QString& did)
{
    auto users = getUserDidList();
    users.removeOne(did);
    mSettings.setValue("users", users);
    clearCredentials(did);

    const auto activeUser = getActiveUser();
    if (did == activeUser)
        setActiveUser("");
}

QString UserSettings::getHost(const QString& did) const
{
    return mSettings.value(key(did, "host")).toString();
}

void UserSettings::savePassword(const QString& did, const QString& password)
{
    const QByteArray encryptedPassword = mEncryption.encrypt(password, KEY_ALIAS_PASSWORD);
    mSettings.setValue(key(did, "password"), encryptedPassword);
}

QString UserSettings::getPassword(const QString& did) const
{
    const QByteArray encryptedPassword = mSettings.value(key(did, "password")).toByteArray();

    if (encryptedPassword.isEmpty())
        return {};

    return mEncryption.decrypt(encryptedPassword, KEY_ALIAS_PASSWORD);
}

QString UserSettings::getHandle(const QString& did) const
{
    return mSettings.value(key(did, "handle")).toString();
}

void UserSettings::saveDisplayName(const QString& did, const QString& displayName)
{
    mSettings.setValue(key(did, "displayName"), displayName);
}

QString UserSettings::getDisplayName(const QString& did) const
{
    return mSettings.value(key(did, "displayName")).toString();
}

void UserSettings::saveAvatar(const QString& did, const QString& avatar)
{
    mSettings.setValue(key(did, "avatar"), avatar);
}

QString UserSettings::getAvatar(const QString& did) const
{
    return mSettings.value(key(did, "avatar")).toString();
}

void UserSettings::saveSession(const QString& did, const ATProto::ComATProtoServer::Session& session)
{
    Q_ASSERT(did == session.mDid);
    const QByteArray encryptedAccessToken = mEncryption.encrypt(session.mAccessJwt, KEY_ALIAS_ACCESS_TOKEN);
    const QByteArray encryptedRefreshToken = mEncryption.encrypt(session.mRefreshJwt, KEY_ALIAS_REFRESH_TOKEN);

    mSettings.setValue(key(did, "handle"), session.mHandle);
    mSettings.setValue(key(did, "access"), encryptedAccessToken);
    mSettings.setValue(key(did, "refresh"), encryptedRefreshToken);
}

ATProto::ComATProtoServer::Session UserSettings::getSession(const QString& did) const
{
    ATProto::ComATProtoServer::Session session;
    session.mDid = did;
    session.mHandle = mSettings.value(key(did, "handle")).toString();
    const QByteArray encryptedAccessToken = mSettings.value(key(did, "access")).toByteArray();

    if (!encryptedAccessToken.isEmpty())
        session.mAccessJwt = mEncryption.decrypt(encryptedAccessToken, KEY_ALIAS_ACCESS_TOKEN);

    const QByteArray encryptedRefreshToken = mSettings.value(key(did, "refresh")).toByteArray();

    if (!encryptedRefreshToken.isEmpty())
        session.mRefreshJwt = mEncryption.decrypt(encryptedRefreshToken, KEY_ALIAS_REFRESH_TOKEN);

    return session;
}

void UserSettings::clearCredentials(const QString& did)
{
    mSettings.setValue(key(did, "password"), "");
    mSettings.setValue(key(did, "access"), "");
    mSettings.setValue(key(did, "refresh"), "");
}

}