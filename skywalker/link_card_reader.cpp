// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#include "link_card_reader.h"
#include "unicode_fonts.h"
#include <QRegularExpression>

namespace Skywalker {

LinkCardReader::LinkCardReader(QObject* parent):
    QObject(parent),
    mCardCache(100)
{
    mNetwork.setAutoDeleteReplies(true);
    mNetwork.setTransferTimeout(15000);
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

void LinkCardReader::getLinkCard(const QString& link)
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
    QNetworkReply* reply = mNetwork.get(request);
    mInProgress = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply]{ extractLinkCard(reply); });
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](auto errCode){ requestFailed(reply, errCode); });
    connect(reply, &QNetworkReply::sslErrors, this, [this, reply]{ requestSslFailed(reply); });
}

void LinkCardReader::extractLinkCard(QNetworkReply* reply)
{
    static const QRegularExpression ogTitleRE(R"(<meta [^>]*(property|name) *=[\"'](og:|twitter:)?title[\"'] [^>]*content=[\"']([^'^\"]+?)[\"'][^>]*>)");
    static const QRegularExpression ogTitleRE2(R"(<meta [^>]*content=[\"']([^'^\"]+?)[\"'] [^>]*(property|name)=[\"'](og:|twitter:)?title[\"'][^>]*>)");
    static const QRegularExpression ogDescriptionRE(R"(<meta [^>]*(property|name) *=[\"'](og:|twitter:)?description[\"'] [^>]*content=[\"']([^'^\"]+?)[\"'][^>]*>)");
    static const QRegularExpression ogDescriptionRE2(R"(<meta [^>]*content=[\"']([^'^\"]+?)[\"'] [^>]*(property|name)=[\"'](og:|twitter:)?description[\"'][^>]*>)");
    static const QRegularExpression ogImageRE(R"(<meta [^>]*(property|name) *=[\"'](og:|twitter:)?image[\"'] [^>]*content=[\"']([^'^\"]+?)[\"'][^>]*>)");
    static const QRegularExpression ogImageRE2(R"(<meta [^>]*content=[\"']([^'^\"]+?)[\"'] [^>]*(property|name)=[\"'](og:|twitter:)?image[\"'][^>]*>)");

    mInProgress = nullptr;

    if (reply->error() != QNetworkReply::NoError)
    {
        requestFailed(reply, reply->error());
        return;
    }

    auto card = std::make_unique<LinkCard>(this);
    const auto data = reply->readAll();
    auto match = ogTitleRE.match(data);

    if (match.hasMatch())
    {
        card->setTitle(UnicodeFonts::toPlainText(match.captured(3)));
    }
    else
    {
        match = ogTitleRE2.match(data);
        if (match.hasMatch())
            card->setTitle(UnicodeFonts::toPlainText(match.captured(1)));
    }

    match = ogDescriptionRE.match(data);

    if (match.hasMatch())
    {
        card->setDescription(UnicodeFonts::toPlainText(match.captured(3)));
    }
    else
    {
        match = ogDescriptionRE2.match(data);
        if (match.hasMatch())
            card->setDescription(UnicodeFonts::toPlainText(match.captured(1)));
    }

    QString imgUrl;
    const auto& url = reply->request().url();
    match = ogImageRE.match(data);

    if (match.hasMatch())
    {
        imgUrl = match.captured(3);
    }
    else
    {
        match = ogImageRE2.match(data);
        if (match.hasMatch())
            imgUrl = match.captured(1);
    }

    if (!imgUrl.isEmpty())
    {
        if (imgUrl.startsWith("/"))
            card->setThumb(url.toString() + imgUrl);
        else
            card->setThumb(imgUrl);
    }

    if (card->isEmpty())
    {
        mCardCache.insert(url, card.release());
        qDebug() << url << "has no link card.";
        return;
    }

    card->setLink(url.toString());
    mCardCache.insert(url, card.get());
    emit linkCard(card.release());
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

}
