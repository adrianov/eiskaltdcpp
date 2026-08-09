/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include "transfergrace/TransferGrace.h"
#include "transferrow/TransferViewItem.h"

#include <QAbstractItemModel>
#include <QMap>
#include <QMultiHash>
#include <QSet>

class TransferViewModel: public QAbstractItemModel
{
    Q_OBJECT

    typedef QVariantMap VarMap;

public:
    TransferViewModel(QObject* = nullptr);
    virtual ~TransferViewModel();

    QVariant data(const QModelIndex &, int) const;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int, int, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    bool hasChildren(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    /** Uploads with a hub match that connection; downloads ignore hub. */
    bool findTransfer(const QString &, bool, TransferViewItem**, const QString &hub = QString());
    /** Download parents key on target; upload parents on target + IP. */
    bool findParent(const QString&, TransferViewItem**, bool = true, const QString &ip = QString());
    TransferViewItem *getParent(const QString &target, const VarMap &params);

    QModelIndex createIndexForItem(TransferViewItem*);

    int getSortColumn() const;
    void setSortColumn(int);
    Qt::SortOrder getSortOrder() const;
    void setSortOrder(Qt::SortOrder);

    void clear();

public Q_SLOTS:
    void repaint();

    void addConnection(const VarMap&);
    void initTransfer(const VarMap&);
    void updateTransfer(const VarMap&);
    void removeTransfer(const VarMap&);
    void removeQueueTarget(const VarMap&);
    /** Drop a download target row now (cancels finished-download grace). */
    void dropQueueTarget(const QString &target);
    void updateTransferPos(const VarMap&, qint64);
    void finishParent(const VarMap&);
    /** Final upload metrics; drop after a short grace if no next Starting. */
    void completeUpload(const VarMap&);
    /** Upload failure: keep error briefly, then drop if Starting never arrives. */
    void failUpload(const VarMap&);
    void updateParents();
    virtual void sort() { sort(sortColumn, sortOrder); }

    void setShowTranferedFilesOnlyState (bool state);
    bool getShowTranferedFilesOnlyState ();

private Q_SLOTS:
    void flushPendingTargetRemoves();
    void pruneUpload(VarMap params);
    void pruneDownload(QString target);

private:
    inline QString      vstr(const QVariant &var) const { return var.toString(); }
    inline qlonglong    vlng(const QVariant &var) const { return var.toLongLong(); }
    inline bool         vbol(const QVariant &var) const { return var.toBool(); }

    void updateParent(TransferViewItem*);
    void pruneEmptyParents();
    bool shouldRemoveStaleRow(const TransferViewItem *item) const;
    void dropTransferRow(TransferViewItem *item);
    void releaseEmptyGroup(TransferViewItem *group);
    TransferViewItem *findUploadRow(const VarMap &params);
    TransferViewItem *transferForUpdate(const VarMap &params);
    TransferViewItem *parentForUpdate(TransferViewItem *item, const VarMap &p,
                                      TransferViewItem *from);
    void applyTransferUpdate(TransferViewItem *item, VarMap &p);
    void placeTransferRow(TransferViewItem *item, const VarMap &p);
    void notifyTransferChange(TransferViewItem *item);
    bool parkDownloadReconnect(const QString &cid);
    bool dropTransferByCid(const QString &cid, bool download, const QString &hub);
    void dropLoneUpload(const QString &cid, const QString &hub);
    void settleUpload(const VarMap &params, bool segmentDone);
    void commitUploadSegment(TransferViewItem *item, TransferViewItem *scope, bool segmentDone);
    void showUploadPartial(TransferViewItem *item, TransferViewItem *scope, const VarMap &params);
    void markUploadFinished(TransferViewItem *item, TransferViewItem *scope);
    void markDownloadComplete(TransferViewItem *item);
    void moveTransfer(TransferViewItem*, TransferViewItem*, TransferViewItem*);
    void removeQueueTargetNow(const QString &target);

    TransferGrace grace;
    QMultiHash<QString, TransferViewItem*> transfer_hash;
    QSet<QString> pendingTargetRemoves;
    bool flushTargetsQueued = false;
    QMap<QString, int> column_map;
    int sortColumn;
    Qt::SortOrder sortOrder;
    TransferViewItem *rootItem;
    bool showTranferedFilesOnly;
    /** Monotonic rank for Download-complete rows (newest first in the list). */
    quint64 finishSeq = 0;
};
