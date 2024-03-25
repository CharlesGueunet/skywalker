// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#include "link_card_reader.h"
#include "unicode_fonts.h"
#include <QRegularExpression>
#include <QNetworkCookie>
#include <QNetworkCookieJar>

namespace Skywalker {

// Store cookies for this session only. No permanent storage!
class CookieJar : public QNetworkCookieJar
{
public:
    QList<QNetworkCookie> cookiesForUrl(const QUrl& url) const override
    {
        qDebug() << "Get cookies for:" << url;
        return QNetworkCookieJar::cookiesForUrl(url);
    }

    bool setCookiesFromUrl(const QList<QNetworkCookie>& cookieList, const QUrl& url) override
    {
        qDebug() << "Set cookies from:" << url;

        for (const auto& cookie : cookieList)
            qDebug() << "Cookie:" << cookie.name() << "=" << cookie.value() << "domain:" << cookie.domain() << "path:" << cookie.path();

        bool retval = QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
        qDebug() << "Cookies accepted:" << retval;
        return retval;
    }

    bool setCookiesFromReply(const QNetworkReply& reply)
    {
        const QVariant setCookie = reply.header(QNetworkRequest::SetCookieHeader);
        if (setCookie.isValid())
        {
            const auto cookies = setCookie.value<QList<QNetworkCookie>>();
            return setCookiesFromUrl(cookies, reply.url());
        }

        return false;
    }
};

LinkCardReader::LinkCardReader(QObject* parent):
    QObject(parent),
    mCardCache(100)
{
    mNetwork.setAutoDeleteReplies(true);
    mNetwork.setTransferTimeout(15000);
    mNetwork.setCookieJar(new CookieJar);
}

LinkCard* LinkCardReader::makeLinkCard(const QString& link, const QString& title,
                       const QString& description, const QString& thumb)
{
    LinkCard* card = new LinkCard(this);
    card->setLink(link);
    card->setTitle(title);
    card->setDescription(description);
    card->setThumb(thumb);

    QUrl url(link);
    mCardCache.insert(url, card);
    return mCardCache[url];
}

void LinkCardReader::getLinkCard(const QString& link, bool retry)
{
    qDebug() << "Get link card:" << link;

    if (mInProgress)
    {
        mInProgress->abort();
        mInProgress = nullptr;
    }

    QString cleanedLink = link.startsWith("http") ? link : "https://" + link;
    if (cleanedLink.endsWith("/"))
        cleanedLink = cleanedLink.sliced(0, cleanedLink.size() - 1);

    QUrl url(cleanedLink);
    if (!url.isValid())
    {
        qWarning() << "Invalid link:" << link;
        return;
    }

    auto* card = mCardCache[url];
    if (card)
    {
        qDebug() << "Got card from cache:" << card->getLink();

        if (!card->isEmpty())
            emit linkCard(card);
        else
            qDebug() << "Card is empty";

        return;
    }

    QNetworkRequest request(url);

    // Without this YouTube Shorts does not load
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, true);

    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::UserVerifiedRedirectPolicy);

    QNetworkReply* reply = mNetwork.get(request);
    mInProgress = reply;
    mPrevDestination = url;
    mRetry = retry;

    connect(reply, &QNetworkReply::finished, this, [this, reply]{ extractLinkCard(reply); });
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](auto errCode){ requestFailed(reply, errCode); });
    connect(reply, &QNetworkReply::sslErrors, this, [this, reply]{ requestSslFailed(reply); });
    connect(reply, &QNetworkReply::redirected, this, [this, reply](const QUrl& url){ redirect(reply, url); });
}

static QString matchRegexes(const std::vector<QRegularExpression>& regexes, const QByteArray& data, const QString& group)
{
    for (const auto& re : regexes)
    {
        auto match = re.match(data);

        if (match.hasMatch())
            return match.captured(group);
    }

    return {};
}

void LinkCardReader::extractLinkCard(QNetworkReply* reply)
{
    static const QString ogTitleStr1(R"(<meta [^>]*(property|name) *=[\"'](og:|twitter:)?title[\"'] [^>]*content=%1(?<title>[^%1]+?)%1[^>]*>)");
    static const QString ogTitleStr2(R"(<meta [^>]*content=%1(?<title>[^%1]+?)%1 [^>]*(property|name)=[\"'](og:|twitter:)?title[\"'][^>]*>)");

    static const std::vector<QRegularExpression> ogTitleREs = {
        QRegularExpression(ogTitleStr1.arg('"')),
        QRegularExpression(ogTitleStr1.arg('\'')),
        QRegularExpression(ogTitleStr2.arg('"')),
        QRegularExpression(ogTitleStr2.arg('\''))
    };

    static const QString ogDescriptionStr1(R"(<meta [^>]*(property|name) *=[\"'](og:|twitter:)?description[\"'] [^>]*content=%1(?<description>[^%1]+?)%1[^>]*>)");
    static const QString ogDescriptionStr2(R"(<meta [^>]*content=%1(?<description>[^%1]+?)%1 [^>]*(property|name)=[\"'](og:|twitter:)?description[\"'][^>]*>)");

    static const std::vector<QRegularExpression> ogDescriptionREs = {
        QRegularExpression(ogDescriptionStr1.arg('"')),
        QRegularExpression(ogDescriptionStr1.arg('\'')),
        QRegularExpression(ogDescriptionStr2.arg('"')),
        QRegularExpression(ogDescriptionStr2.arg('\''))
    };

    static const QString ogImageStr1(R"(<meta [^>]*(property|name) *=[\"'](og:|twitter:)?image[\"'] [^>]*content=%1(?<image>[^%1]+?)%1[^>]*>)");
    static const QString ogImageStr2(R"(<meta [^>]*content=%1(?<image>[^%1]+?)%1 [^>]*(property|name)=[\"'](og:|twitter:)?image[\"'][^>]*>)");

    static const std::vector<QRegularExpression> ogImageREs = {
        QRegularExpression(ogImageStr1.arg('"')),
        QRegularExpression(ogImageStr1.arg('\'')),
        QRegularExpression(ogImageStr2.arg('"')),
        QRegularExpression(ogImageStr2.arg('\''))
    };

    mInProgress = nullptr;

    if (reply->error() != QNetworkReply::NoError)
    {
        requestFailed(reply, reply->error());
        return;
    }

    auto card = std::make_unique<LinkCard>(this);
    const auto data = reply->readAll();

    const QString title = matchRegexes(ogTitleREs, data, "title");
    if (!title.isEmpty())
        card->setTitle(toPlainText(title));

    const QString description = matchRegexes(ogDescriptionREs, data, "description");
    if (!description.isEmpty())
        card->setDescription(toPlainText(description));

    const QString imgUrlString = matchRegexes(ogImageREs, data, "image");
    const auto& url = reply->request().url();

    if (!imgUrlString.isEmpty())
    {
        QUrl imgUrl(imgUrlString);

        if (imgUrl.isRelative())
        {
            if (QDir::isAbsolutePath(imgUrlString))
            {
                imgUrl.setHost(url.host());
                imgUrl.setScheme(url.scheme());
                imgUrl.setPort(url.port());
                card->setThumb(imgUrl.toString());
            }
            else
            {
                card->setThumb(url.toString() + imgUrlString);
            }
        }
        else
        {
            card->setThumb(imgUrlString);
        }
    }

    if (card->isEmpty())
    {
        auto* cookieJar = static_cast<CookieJar*>(mNetwork.cookieJar());
        Q_ASSERT(cookieJar);

        if (!mRetry && cookieJar->setCookiesFromReply(*reply))
        {
            qDebug() << "Cookies stored, retry";
            getLinkCard(url.toString(), true);
        }
        else
        {
            mCardCache.insert(url, card.release());
            qDebug() << url << "has no link card.";
        }

        return;
    }

    card->setLink(url.toString());
    mCardCache.insert(url, card.get());
    emit linkCard(card.release());
}

QString LinkCardReader::toPlainText(const QString& text)
{
    // Texts in linkcard text often contain double encoded ampersands, e.g.
    // &amp;#8216;
    // That should be &#8216;
    QString plain(text);
    plain.replace("&amp;#", "&#");
    return UnicodeFonts::toPlainText(plain);
}

void LinkCardReader::requestFailed(QNetworkReply* reply, int errCode)
{
    mInProgress = nullptr;
    qDebug() << "Failed to get link:" << reply->request().url();
    qDebug() << "Error:" << errCode << reply->errorString();
}

void LinkCardReader::requestSslFailed(QNetworkReply* reply)
{
    mInProgress = nullptr;
    qDebug() << "SSL error, failed to get link:" << reply->request().url();
}

void LinkCardReader::redirect(QNetworkReply* reply, const QUrl& redirectUrl)
{
    qDebug() << "Prev url:" << mPrevDestination << "redirect url:" << redirectUrl;

    // Allow: https -> https, http -> http, http -> https
    // Allow https -> http only if the host stays the same
    if (mPrevDestination.scheme() == redirectUrl.scheme() ||
        mPrevDestination.scheme() == "http" ||
        mPrevDestination.host() == redirectUrl.host())
    {
        emit reply->redirectAllowed();
    }

    auto* cookieJar = static_cast<CookieJar*>(mNetwork.cookieJar());
    Q_ASSERT(cookieJar);
    cookieJar->setCookiesFromReply(*reply);

    mPrevDestination = redirectUrl;
}

}
