// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#pragma once
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Skywalker {

class ImageReader : public QObject
{
public:
    using ImageCb = std::function<void(QImage)>;
    using ErrorCb = std::function<void(const QString& error)>;

    ImageReader();
    void getImage(const QString& urlString, const ImageCb& imageCb, const ErrorCb& errorCb);

private:
    void replyFinished(QNetworkReply* reply, const ImageCb& imageCb, const ErrorCb& errorCb);

    QNetworkAccessManager mNetwork;
};

}
