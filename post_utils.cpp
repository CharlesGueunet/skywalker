// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#include "post_utils.h"
#include "jni_callback.h"
#include "photo_picker.h"

namespace Skywalker {

PostUtils::PostUtils(QObject* parent) :
    QObject(parent)
{
    auto& jniCallbackListener = JNICallbackListener::getInstance();
    QObject::connect(&jniCallbackListener, &JNICallbackListener::photoPicked,
                     this, [this](const QString& uri){
                         qDebug() << "PHOTO PICKED:" << uri;
                         QString fileName = resolveContentUriToFile(uri);
                         qDebug() << "PHOTO FILE NAME:" << fileName;
                         QFile file(fileName);
                         qDebug() << "File exists:" << file.exists() << ",size:" << file.size();
                         emit photoPicked(fileName);
                     });
}

ATProto::Client* PostUtils::bskyClient()
{
    Q_ASSERT(mSkywalker);
    auto* client = mSkywalker->getBskyClient();
    Q_ASSERT(client);
    return client;
}

ImageReader* PostUtils::imageReader()
{
    if (!mImageReader)
        mImageReader = std::make_unique<ImageReader>();

    return mImageReader.get();
}

void PostUtils::setSkywalker(Skywalker* skywalker)
{
    mSkywalker = skywalker;
    emit skywalkerChanged();
}

void PostUtils::post(QString text, const QStringList& imageFileNames)
{
    Q_ASSERT(mSkywalker);
    qDebug() << "Posting:" << text;

    bskyClient()->createPost(text, [this, imageFileNames](auto post){
        continuePost(imageFileNames, post); });
}

void PostUtils::continuePost(const QStringList& imageFileNames, ATProto::AppBskyFeed::Record::Post::SharedPtr post, int imgIndex)
{
    if (imgIndex >= imageFileNames.size())
    {
        continuePost(post);
        return;
    }

    const auto& fileName = imageFileNames[imgIndex];
    const auto& blob = createBlob(fileName);
    if (blob.isEmpty())
    {
        const QString error = tr("Could not load image") + ": " + QFileInfo(fileName).fileName();
        emit postFailed(error);
        return;
    }

    emit postProgress(tr("Uploading image") + QString(" #%1").arg(imgIndex + 1));

    bskyClient()->uploadBlob(blob, "image/jpeg",
        [this, imageFileNames, post, imgIndex](auto blob){
            bskyClient()->addImageToPost(*post, std::move(blob));
            continuePost(imageFileNames, post, imgIndex + 1);
        },
        [this](const QString& error){
            qDebug() << "Post failed:" << error;
            emit postFailed(error);
        });
}

void PostUtils::post(QString text, const LinkCard* card)
{
    Q_ASSERT(card);
    Q_ASSERT(mSkywalker);
    qDebug() << "Posting:" << text;

    bskyClient()->createPost(text, [this, card](auto post){
        continuePost(card, post); });
}

void PostUtils::continuePost(const LinkCard* card, ATProto::AppBskyFeed::Record::Post::SharedPtr post)
{
    Q_ASSERT(card);
    if (card->getThumb().isEmpty())
    {
        continuePost(card, QImage(), post);
        return;
    }

    emit postProgress(tr("Retrieving card image"));

    imageReader()->getImage(card->getThumb(),
        [this, card, post](auto image){
            continuePost(card, image, post);
        },
        [this](const QString& error){
            qDebug() << "Post failed:" << error;
            emit postFailed(error);
        });
}

void PostUtils::continuePost(const LinkCard* card, QImage thumb, ATProto::AppBskyFeed::Record::Post::SharedPtr post)
{
    Q_ASSERT(card);
    QByteArray blob;

    if (!thumb.isNull())
        blob = createBlob(thumb, card->getThumb());

    if (blob.isEmpty())
    {
        bskyClient()->addExternalToPost(*post, card->getLink(), card->getTitle(), card->getDescription());
        continuePost(post);
        return;
    }

    emit postProgress(tr("Uploading card image"));

    bskyClient()->uploadBlob(blob, "image/jpeg",
        [this, card, post](auto blob){
            bskyClient()->addExternalToPost(*post, card->getLink(), card->getTitle(),
                    card->getDescription(), std::move(blob));
            continuePost(post);
        },
        [this](const QString& error){
            qDebug() << "Post failed:" << error;
            emit postFailed(error);
        });
}

void PostUtils::continuePost(ATProto::AppBskyFeed::Record::Post::SharedPtr post)
{
    emit postProgress(tr("Posting"));
    bskyClient()->post(*post,
        [this]{
            emit postOk();
        },
        [this](const QString& error){
            qDebug() << "Post failed:" << error;
            emit postFailed(error);
        });
}

void PostUtils::pickPhoto() const
{
    ::Skywalker::pickPhoto();
}

void PostUtils::setFirstWebLink(const QString& link)
{
    if (link == mFirstWebLink)
        return;

    mFirstWebLink = link;
    emit firstWebLinkChanged();
}

QString PostUtils::highlightMentionsAndLinks(const QString& text)
{
    const auto facets = bskyClient()->parseFacets(text);
    QString highlighted;
    int pos = 0;
    bool webLinkFound = false;
    mLinkShorteningReduction = 0;

    for (const auto& facet : facets)
    {
        if (facet.mType == ATProto::Client::ParsedMatch::Type::LINK)
        {
            if (!webLinkFound)
            {
                setFirstWebLink(facet.mMatch);
                webLinkFound = true;
            }

            const auto shortLink = ATProto::Client::shortenWebLink(facet.mMatch);
            const int reduction = graphemeLength(facet.mMatch) - graphemeLength(shortLink);
            qDebug() << "SHORT:" << shortLink << "reduction:" << reduction;
            mLinkShorteningReduction += reduction;
        }

        const auto before = text.sliced(pos, facet.mStartIndex - pos);
        highlighted.append(before.toHtmlEscaped());
        QString highlight = QString("<font color=\"blue\">%1</font>").arg(facet.mMatch);
        highlighted.append(highlight);
        pos = facet.mEndIndex;
    }

    if (!webLinkFound)
        setFirstWebLink(QString());

    highlighted.append(text.sliced(pos).toHtmlEscaped());
    return highlighted;
}

int PostUtils::graphemeLength(const QString& text) const
{
    QTextBoundaryFinder boundaryFinder(QTextBoundaryFinder::Grapheme, text);
    int length = 0;

    while (boundaryFinder.toNextBoundary() != -1)
        ++length;

    return length;
}

// In the Post Compose page the text area is in HTML.
// In the plain text we get some cruft is still left.
QString PostUtils::cleanText(QString text)
{
    return text.replace(QChar(QChar::ParagraphSeparator), "\n");
}

}