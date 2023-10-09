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
    Q_PROPERTY(SvgImage addImage MEMBER sAddPhotoAlternate CONSTANT FINAL)
    Q_PROPERTY(SvgImage arrowBack MEMBER sArrowBack CONSTANT FINAL)
    Q_PROPERTY(SvgImage block MEMBER sBlock CONSTANT FINAL)
    Q_PROPERTY(SvgImage chat MEMBER sChat CONSTANT FINAL)
    Q_PROPERTY(SvgImage close MEMBER sClose CONSTANT FINAL)
    Q_PROPERTY(SvgImage hideVisibility MEMBER sVisibilityOff CONSTANT FINAL)
    Q_PROPERTY(SvgImage home MEMBER sHome CONSTANT FINAL)
    Q_PROPERTY(SvgImage like MEMBER sFavorite CONSTANT FINAL)
    Q_PROPERTY(SvgImage moreVert MEMBER sMoreVert CONSTANT FINAL)
    Q_PROPERTY(SvgImage mute MEMBER sDoNotDisturbOn CONSTANT FINAL)
    Q_PROPERTY(SvgImage noPosts MEMBER sSpeakerNotesOff CONSTANT FINAL)
    Q_PROPERTY(SvgImage notifications MEMBER sNotifications CONSTANT FINAL)
    Q_PROPERTY(SvgImage reply MEMBER sReply CONSTANT FINAL)
    Q_PROPERTY(SvgImage repost MEMBER sRepeat CONSTANT FINAL)
    QML_ELEMENT

public:
    explicit SvgOutline(QObject* parent = nullptr) : QObject(parent) {}

private:
    // fonts.googlecom weight=100, grade=0, optical size=24px
    static constexpr SvgImage sAddPhotoAlternate{"M212-172q-24.75 0-42.375-17.625T152-232v-496q0-24.75 17.625-42.375T212-788h328v28H212q-14 0-23 9t-9 23v496q0 14 9 23t23 9h496q14 0 23-9t9-23v-328h28v328q0 24.75-17.625 42.375T708-172H212Zm488-468v-80h-80v-28h80v-80h28v80h80v28h-80v80h-28ZM298-306h332L528-442 428-318l-64-74-66 86ZM180-760v560-560Z"};
    static constexpr SvgImage sArrowBack{"m266-466 234 234-20 20-268-268 268-268 20 20-234 234h482v28H266Z"};
    static constexpr SvgImage sBlock{"M480.174-132Q408-132 344.442-159.391q-63.559-27.392-110.575-74.348-47.015-46.957-74.441-110.435Q132-407.652 132-479.826q0-72.174 27.391-135.732 27.392-63.559 74.348-110.574 46.957-47.016 110.435-74.442Q407.652-828 479.826-828q72.174 0 135.732 27.391 63.559 27.392 110.574 74.348 47.016 46.957 74.442 110.435Q828-552.348 828-480.174q0 72.174-27.391 135.732-27.392 63.559-74.348 110.575-46.957 47.015-110.435 74.441Q552.348-132 480.174-132ZM480-160q59.961 0 115.481-21.5Q651-203 696-244L244-696q-40 45-62 100.519-22 55.52-22 115.481 0 134 93 227t227 93Zm236-104q41-45 62.5-100.519Q800-420.039 800-480q0-134-93-227t-227-93q-60.312 0-116.156 21Q308-758 264-716l452 452Z"};
    static constexpr SvgImage sChat{"M266-426h268v-28H266v28Zm0-120h428v-28H266v28Zm0-120h428v-28H266v28ZM132-180v-588q0-26 17-43t43-17h576q26 0 43 17t17 43v416q0 26-17 43t-43 17H244L132-180Zm100-140h536q12 0 22-10t10-22v-416q0-12-10-22t-22-10H192q-12 0-22 10t-10 22v520l72-72Zm-72 0v-480 480Z"};
    static constexpr SvgImage sClose{"m256-236-20-20 224-224-224-224 20-20 224 224 224-224 20 20-224 224 224 224-20 20-224-224-224 224Z"};
    static constexpr SvgImage sDoNotDisturbOn{"M306-466h348v-28H306v28Zm174.174 334Q408-132 344.442-159.391q-63.559-27.392-110.575-74.348-47.015-46.957-74.441-110.435Q132-407.652 132-479.826q0-72.174 27.391-135.732 27.392-63.559 74.348-110.574 46.957-47.016 110.435-74.442Q407.652-828 479.826-828q72.174 0 135.732 27.391 63.559 27.392 110.574 74.348 47.016 46.957 74.442 110.435Q828-552.348 828-480.174q0 72.174-27.391 135.732-27.392 63.559-74.348 110.575-46.957 47.015-110.435 74.441Q552.348-132 480.174-132ZM480-160q134 0 227-93t93-227q0-134-93-227t-227-93q-134 0-227 93t-93 227q0 134 93 227t227 93Zm0-320Z"};
    static constexpr SvgImage sFavorite{"m480-190-22-20q-97-89-160.5-152t-100-110.5Q161-520 146.5-558T132-634q0-71 48.5-119.5T300-802q53 0 99 28.5t81 83.5q35-55 81-83.5t99-28.5q71 0 119.5 48.5T828-634q0 38-14.5 76t-51 85.5Q726-425 663-362T502-210l-22 20Zm0-38q96-87 158-149t98-107.5q36-45.5 50-80.5t14-69q0-60-40-100t-100-40q-48 0-88.5 27.5T494-660h-28q-38-60-78-87t-88-27q-59 0-99.5 40T160-634q0 34 14 69t50 80.5q36 45.5 98 107T480-228Zm0-273Z"};
    static constexpr SvgImage sHome{"M240-200h156v-234h168v234h156v-360L480-742 240-560v360Zm-28 28v-402l268-203 268 203v402H536v-234H424v234H212Zm268-299Z"};
    static constexpr SvgImage sMoreVert{"M480-236q-11.55 0-19.775-8.225Q452-252.45 452-264q0-11.55 8.225-19.775Q468.45-292 480-292q11.55 0 19.775 8.225Q508-275.55 508-264q0 11.55-8.225 19.775Q491.55-236 480-236Zm0-216q-11.55 0-19.775-8.225Q452-468.45 452-480q0-11.55 8.225-19.775Q468.45-508 480-508q11.55 0 19.775 8.225Q508-491.55 508-480q0 11.55-8.225 19.775Q491.55-452 480-452Zm0-216q-11.55 0-19.775-8.225Q452-684.45 452-696q0-11.55 8.225-19.775Q468.45-724 480-724q11.55 0 19.775 8.225Q508-707.55 508-696q0 11.55-8.225 19.775Q491.55-668 480-668Z"};
    static constexpr SvgImage sNotifications{"M212-212v-28h60v-328q0-77 49.5-135T446-774v-20q0-14.167 9.882-24.083 9.883-9.917 24-9.917Q494-828 504-818.083q10 9.916 10 24.083v20q75 13 124.5 71T688-568v328h60v28H212Zm268-282Zm-.177 382Q455-112 437.5-129.625T420-172h120q0 25-17.677 42.5t-42.5 17.5ZM300-240h360v-328q0-75-52.5-127.5T480-748q-75 0-127.5 52.5T300-568v328Z"};
    static constexpr SvgImage sRepeat{"M290-126 166-250l124-124 20 20-90 90h476v-160h28v188H220l90 90-20 20Zm-54-410v-188h504l-90-90 20-20 124 124-124 124-20-20 90-90H264v160h-28Z"};
    static constexpr SvgImage sReply{"M760-252v-108q0-60-43-103t-103-43H226l170 170-20 20-204-204 204-204 20 20-170 170h388q72 0 123 51t51 123v108h-28Z"};
    static constexpr SvgImage sSpeakerNotesOff{"M288-412q-11 0-19.5-8t-8.5-20q0-12 8.5-20t19.5-8q11 0 19.5 8t8.5 20q0 12-8.5 20t-19.5 8Zm488 120-22-28h14q14 0 23-9t9-23v-416q0-14-9-23t-23-9H274l-28-28h522q26 0 43 17t17 43v416q0 26-14.5 40.5T776-292ZM528-546l-28-28h194v28H528Zm280 434L628-292H244L132-180v-608l-88-88 20-20 764 764-20 20ZM380-540Zm134-20Zm-226 28q-11 0-19.5-8t-8.5-20q0-12 8.5-20t19.5-8q11 0 19.5 8t8.5 20q0 12-8.5 20t-19.5 8Zm120-134-11-11v-17h297v28H408Zm-248-94v512l72-72h368L160-760Z"};
    static constexpr SvgImage sVisibilityOff{"m610-462-24-24q9-54-30.5-91.5T466-606l-24-24q8-3 17-4.5t21-1.5q57 0 96.5 39.5T616-500q0 12-1.5 22t-4.5 16Zm126 122-22-18q38-29 67.5-63.5T832-500q-50-101-143.5-160.5T480-720q-29 0-57 4t-55 12l-22-22q33-12 67-17t67-5q124 0 229 68t155 180q-21 45-52.5 85T736-340Zm52 208L636-284q-24 12-64.5 22T480-252q-125 0-229-68T96-500q24-53 64-99.5t84-76.5L132-788l20-20 656 656-20 20ZM264-656q-36 24-75.5 66.5T128-500q50 101 143.5 160.5T480-280q39 0 79-8t57-16l-74-74q-9 6-28 10t-34 4q-57 0-96.5-39.5T344-500q0-14 4-32.5t10-29.5l-94-94Zm277 125Zm-101 51Z"};
};

}
