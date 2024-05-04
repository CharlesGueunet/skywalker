// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#include "content_filter.h"
#include <atproto/lib/rich_text_master.h>

namespace Skywalker {

// We are implicitly subscribed to the Bluesky moderator
#define BLUESKY_MODERATOR_DID QStringLiteral("did:plc:ar7c4by46qjdydhdevvrndac")

const std::vector<ContentGroup> ContentFilter::CONTENT_GROUP_LIST = {
    {
        "porn",
        QObject::tr("Adult Content"),
        QObject::tr("Explicit sexual images"),
        "nsfw",
        true,
        QEnums::CONTENT_VISIBILITY_WARN_MEDIA,
        QEnums::LABEL_TARGET_MEDIA,
        ""
    },
    {
        "nudity",
        QObject::tr("Non-sexual Nudity"),
        QObject::tr("E.g. artistic nudes"),
        {},
        true,
        QEnums::CONTENT_VISIBILITY_WARN_MEDIA,
        QEnums::LABEL_TARGET_MEDIA,
        ""
    },
    {
        "sexual",
        QObject::tr("Sexually Suggestive"),
        QObject::tr("Does not include nudity"),
        "suggestive",
        true,
        QEnums::CONTENT_VISIBILITY_WARN_MEDIA,
        QEnums::LABEL_TARGET_MEDIA,
        ""
    },
    {
        "graphic-media",
        QObject::tr("Graphic Media"),
        QObject::tr("Explicit or potentially disturbing media"),
        "gore",
        true,
        QEnums::CONTENT_VISIBILITY_HIDE_MEDIA,
        QEnums::LABEL_TARGET_MEDIA,
        ""
    }
};

ContentFilter::GlobalContentGroupMap ContentFilter::CONTENT_GROUPS;

void ContentFilter::initContentGroups()
{
    for (const auto& group : CONTENT_GROUP_LIST)
        CONTENT_GROUPS[group.getLabelId()] = &group;
}

const ContentFilter::GlobalContentGroupMap& ContentFilter::getContentGroups()
{
    if (CONTENT_GROUPS.empty())
        initContentGroups();

    return CONTENT_GROUPS;
}

const ContentGroup* ContentFilter::getGlobalContentGroup(const QString& labelId)
{
    const auto& groups = getContentGroups();
    auto it = groups.find(labelId);
    return it != groups.end() ? it->second : nullptr;
}

bool ContentFilter::isGlobalLabel(const QString& labelId)
{
    return getGlobalContentGroup(labelId) != nullptr;
}

QString ContentGroup::getFormattedDescription() const
{
    return ATProto::RichTextMaster::plainToHtml(mDescription);
}

QEnums::ContentVisibility ContentGroup::getContentVisibility(ATProto::UserPreferences::LabelVisibility visibility) const
{
    switch (visibility)
    {
    case ATProto::UserPreferences::LabelVisibility::SHOW:
        return QEnums::CONTENT_VISIBILITY_SHOW;
    case ATProto::UserPreferences::LabelVisibility::WARN:
        return isPostLevel() ? QEnums::CONTENT_VISIBILITY_WARN_POST : QEnums::CONTENT_VISIBILITY_WARN_MEDIA;
    case ATProto::UserPreferences::LabelVisibility::HIDE:
        return isPostLevel() ? QEnums::CONTENT_VISIBILITY_HIDE_POST : QEnums::CONTENT_VISIBILITY_HIDE_MEDIA;
    case ATProto::UserPreferences::LabelVisibility::UNKNOWN:
        Q_ASSERT(false);
        return QEnums::CONTENT_VISIBILITY_SHOW;
    }

    Q_ASSERT(false);
    return QEnums::CONTENT_VISIBILITY_SHOW;
}

std::unordered_map<QString, QString> ContentFilter::sLabelGroupMap;

ContentFilter::ContentFilter(const ATProto::UserPreferences& userPreferences) :
    mUserPreferences(userPreferences)
{
    if (sLabelGroupMap.empty())
        initLabelGroupMap();
}

void ContentFilter::initLabelGroupMap()
{
    for (const auto& [id, group] : getContentGroups())
    {
        sLabelGroupMap[id] = id;

        if (group->getLegacyLabelId())
            sLabelGroupMap[*group->getLegacyLabelId()] = id;
    }
}

ContentLabelList ContentFilter::getContentLabels(const LabelList& labels)
{
    ContentLabelList contentLabels;

    for (const auto& label : labels)
    {
        const ContentLabel contentLabel(label->mSrc, label->mVal, label->mCreatedAt);

        if (!label->mNeg)
        {
            contentLabels.append(contentLabel);
        }
        else
        {
            contentLabels.removeIf([&contentLabel](const ContentLabel& l){
                return l.getLabelId() == contentLabel.getLabelId() && l.getDid() == contentLabel.getDid();
            });
        }
    }

    return contentLabels;
}

QEnums::ContentVisibility ContentFilter::getGroupVisibility(const ContentGroup& group) const
{
    if (group.isAdult() && !mUserPreferences.getAdultContent())
        return QEnums::CONTENT_VISIBILITY_HIDE_MEDIA;

    auto visibility = mUserPreferences.getLabelVisibility(group.getLabelerDid(), group.getLabelId());

    if (visibility == ATProto::UserPreferences::LabelVisibility::UNKNOWN && group.getLegacyLabelId())
        visibility = mUserPreferences.getLabelVisibility(group.getLabelerDid(), *group.getLegacyLabelId());

    if (visibility != ATProto::UserPreferences::LabelVisibility::UNKNOWN)
        return group.getContentVisibility(visibility);

    return group.getDefaultVisibility();
}

QEnums::ContentVisibility ContentFilter::getVisibility(const ContentLabel& label) const
{
    auto it = sLabelGroupMap.find(label.getLabelId());

    if (it != sLabelGroupMap.end())
    {
        const auto& groupId = it->second;
        const auto& group = getContentGroups().at(groupId);
        return getGroupVisibility(*group);
    }

    auto labelerIt = mLabelerGroupMap.find(label.getDid());

    if (labelerIt != mLabelerGroupMap.end())
    {
        const auto& groupMap = labelerIt->second;
        auto groupIt = groupMap.find(label.getLabelId());

        if (groupIt != groupMap.end())
            return getGroupVisibility(groupIt->second);
    }


    qDebug() << "Undefined label:" << label.getLabelId() << "labeler:" << label.getDid();
    return QEnums::CONTENT_VISIBILITY_SHOW;
}

QString ContentFilter::getGroupWarning(const ContentGroup& group) const
{
    if (group.isAdult() && !mUserPreferences.getAdultContent())
        return QObject::tr("Adult content");

    return group.getTitle();
}

QString ContentFilter::getWarning(const ContentLabel& label) const
{
    auto it = sLabelGroupMap.find(label.getLabelId());

    if (it != sLabelGroupMap.end())
    {
        const auto& groupId = it->second;
        const auto& group = getContentGroups().at(groupId);
        return getGroupWarning(*group);
    }

    auto labelerIt = mLabelerGroupMap.find(label.getDid());

    if (labelerIt != mLabelerGroupMap.end())
    {
        const auto& groupMap = labelerIt->second;
        auto groupIt = groupMap.find(label.getLabelId());

        if (groupIt != groupMap.end())
            return getGroupWarning(groupIt->second);
    }

    qDebug() << "Undefined label:" << label.getLabelId() << "labeler:" << label.getDid();
    return QObject::tr("Unknown label") + QString(": %1").arg(label.getLabelId());
}

std::tuple<QEnums::ContentVisibility, QString> ContentFilter::getVisibilityAndWarning(const ATProto::ComATProtoLabel::LabelList& labels) const
{
    const auto contentLabels = getContentLabels(labels);
    return getVisibilityAndWarning(contentLabels);
}

std::tuple<QEnums::ContentVisibility, QString> ContentFilter::getVisibilityAndWarning(const ContentLabelList& contentLabels) const
{
    QEnums::ContentVisibility visibility = QEnums::CONTENT_VISIBILITY_SHOW;
    QString warning;

    for (const auto& label : contentLabels)
    {
        const auto v = getVisibility(label);

        if (v <= visibility)
            continue;

        visibility = v;
        warning = getWarning(label);
    }

    return {visibility, warning};
}

bool ContentFilter::isSubscribedToLabeler(const QString& did) const
{
    if (isFixedLabelerSubscription(did))
        return true;

    ATProto::AppBskyActor::LabelerPrefItem item;
    item.mDid = did;
    const auto& prefs = mUserPreferences.getLabelersPref();
    return prefs.mLabelers.contains(item);
}

std::unordered_set<QString> ContentFilter::getSubscribedLabelerDids() const
{
    auto dids = mUserPreferences.getLabelerDids();
    dids.insert(BLUESKY_MODERATOR_DID);
    return dids;
}

size_t ContentFilter::numLabelers() const
{
    return mUserPreferences.numLabelers() + 1; // +1 for Bluesky labeler
}

bool ContentFilter::isFixedLabelerSubscription(const QString& did)
{
    return did == BLUESKY_MODERATOR_DID;
}

void ContentFilter::addContentGroupMap(const QString& did, const ContentGroupMap& contentGroupMap)
{
    Q_ASSERT(!did.isEmpty());
    qDebug() << "Add content group map for did:" << did;
    mLabelerGroupMap[did] = contentGroupMap;
}

void ContentFilter::addContentGroups(const QString& did, const std::vector<ContentGroup>& contentGroups)
{
    Q_ASSERT(!did.isEmpty());
    qDebug() << "Add content groups for did:" << did;
    auto& groupMap = mLabelerGroupMap[did];

    for (const auto& group : contentGroups)
        groupMap[group.getLabelId()] = group;
}

void ContentFilter::removeContentGroups(const QString& did)
{
    Q_ASSERT(!did.isEmpty());
    mLabelerGroupMap.erase(did);
}

}
