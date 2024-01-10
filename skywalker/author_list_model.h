// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#pragma once
#include "content_filter.h"
#include "enums.h"
#include "local_author_model_changes.h"
#include "profile.h"
#include <QAbstractListModel>
#include <deque>

namespace Skywalker {

class AuthorListModel : public QAbstractListModel, public LocalAuthorModelChanges
{
    Q_OBJECT
public:
    enum class Role {
        Author = Qt::UserRole + 1,
        FollowingUri,
        BlockingUri,
        ListItemUri
    };

    struct ListEntry
    {
        Profile mProfile;
        QString mListItemUri; // empty when not part of a list

        explicit ListEntry(const Profile& profile, const QString& listItemUri = {});
    };

    using Type = QEnums::AuthorListType;
    using Ptr = std::unique_ptr<AuthorListModel>;

    AuthorListModel(Type type, const QString& atId, const ContentFilter& contentFilter, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void clear();
    void addAuthors(ATProto::AppBskyActor::ProfileViewList authors, const QString& cursor);
    void addAuthors(ATProto::AppBskyGraph::ListItemViewList listItems, const QString& cursor);
    Q_INVOKABLE void prependAuthor(const Profile& author, const QString& listItemUri);
    const QString& getCursor() const { return mCursor; }
    bool isEndOfList() const { return mCursor.isEmpty(); }

    Type getType() const { return mType; }
    const QString& getAtId() const { return mAtId; }

protected:
    QHash<int, QByteArray> roleNames() const override;

    virtual void blockingUriChanged() override;
    virtual void followingUriChanged() override;

private:
    using AuthorList = std::deque<ListEntry>;

    AuthorList filterAuthors(const ATProto::AppBskyActor::ProfileViewList& authors) const;
    void changeData(const QList<int>& roles);

    Type mType;
    QString mAtId;
    const ContentFilter& mContentFilter;

    AuthorList mList;
    std::deque<ATProto::AppBskyActor::ProfileViewList> mRawLists;
    std::deque<ATProto::AppBskyGraph::ListItemViewList> mRawItemLists;

    QString mCursor;
};

}
