// Copyright (C) 2023 Michel de Boer

// License: GPLv3
#pragma once
#include "svg_image.h"
#include <QObject>
#include <QtQmlIntegration>

namespace Skywalker {

class SvgOutline : public QObject
{
    Q_OBJECT
    Q_PROPERTY(SvgImage arrowBack MEMBER sArrowBack CONSTANT FINAL)
    Q_PROPERTY(SvgImage home MEMBER sHome CONSTANT FINAL)
    Q_PROPERTY(SvgImage like MEMBER sFavorite CONSTANT FINAL)
    Q_PROPERTY(SvgImage moreVert MEMBER sMoreVert CONSTANT FINAL)
    Q_PROPERTY(SvgImage reply MEMBER sReply CONSTANT FINAL)
    Q_PROPERTY(SvgImage repost MEMBER sRepeat CONSTANT FINAL)
    QML_ELEMENT

public:
    explicit SvgOutline(QObject* parent = nullptr) : QObject(parent) {}

private:
    // fonts.googlecom weight=100, grade=0, optical size=24px
    static constexpr SvgImage sArrowBack{"m266-466 234 234-20 20-268-268 268-268 20 20-234 234h482v28H266Z"};
    static constexpr SvgImage sFavorite{"m480-190-22-20q-97-89-160.5-152t-100-110.5Q161-520 146.5-558T132-634q0-71 48.5-119.5T300-802q53 0 99 28.5t81 83.5q35-55 81-83.5t99-28.5q71 0 119.5 48.5T828-634q0 38-14.5 76t-51 85.5Q726-425 663-362T502-210l-22 20Zm0-38q96-87 158-149t98-107.5q36-45.5 50-80.5t14-69q0-60-40-100t-100-40q-48 0-88.5 27.5T494-660h-28q-38-60-78-87t-88-27q-59 0-99.5 40T160-634q0 34 14 69t50 80.5q36 45.5 98 107T480-228Zm0-273Z"};
    static constexpr SvgImage sHome{"M240-200h156v-234h168v234h156v-360L480-742 240-560v360Zm-28 28v-402l268-203 268 203v402H536v-234H424v234H212Zm268-299Z"};
    static constexpr SvgImage sMoreVert{"M480-236q-11.55 0-19.775-8.225Q452-252.45 452-264q0-11.55 8.225-19.775Q468.45-292 480-292q11.55 0 19.775 8.225Q508-275.55 508-264q0 11.55-8.225 19.775Q491.55-236 480-236Zm0-216q-11.55 0-19.775-8.225Q452-468.45 452-480q0-11.55 8.225-19.775Q468.45-508 480-508q11.55 0 19.775 8.225Q508-491.55 508-480q0 11.55-8.225 19.775Q491.55-452 480-452Zm0-216q-11.55 0-19.775-8.225Q452-684.45 452-696q0-11.55 8.225-19.775Q468.45-724 480-724q11.55 0 19.775 8.225Q508-707.55 508-696q0 11.55-8.225 19.775Q491.55-668 480-668Z"};
    static constexpr SvgImage sRepeat{"M290-126 166-250l124-124 20 20-90 90h476v-160h28v188H220l90 90-20 20Zm-54-410v-188h504l-90-90 20-20 124 124-124 124-20-20 90-90H264v160h-28Z"};
    static constexpr SvgImage sReply{"M760-252v-108q0-60-43-103t-103-43H226l170 170-20 20-204-204 204-204 20 20-170 170h388q72 0 123 51t51 123v108h-28Z"};
};

}
