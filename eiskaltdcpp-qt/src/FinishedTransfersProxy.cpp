/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfersProxy.h"
#include "FinishedTransfersModel.h"
#include "SearchFileTypes.h"

#include "dcpp/stdinc.h"
#include "dcpp/Util.h"
#include "WulforUtil.h"

#include <QFileInfo>

using namespace dcpp;

namespace {

QString cell(const QAbstractItemModel *m, int row, int col, const QModelIndex &parent)
{
    return m->data(m->index(row, col, parent), Qt::DisplayRole).toString();
}

bool rowIsFull(const QAbstractItemModel *m, int row, const QModelIndex &parent)
{
    const int fullCol = m->index(row, COLUMN_FINISHED_FULL, parent).isValid()
            ? COLUMN_FINISHED_FULL : COLUMN_FINISHED_CRC32;
    return cell(m, row, fullCol, parent) == QLatin1String("1");
}

bool fileListOk(bool hideLists, bool requireFull,
                const QAbstractItemModel *m, int row, const QModelIndex &parent)
{
    if (!hideLists && !requireFull)
        return true;
    if (hideLists) {
        const string target = _tq(cell(m, row, COLUMN_FINISHED_TARGET, parent));
        const string path = _tq(cell(m, row, COLUMN_FINISHED_PATH, parent) +
                                cell(m, row, COLUMN_FINISHED_NAME, parent));
        if (isFinishedFileList(target) || isFinishedFileList(path))
            return false;
    }
    return !requireFull || rowIsFull(m, row, parent);
}

bool typeOk(bool fileView, const QStringList &exts, bool adultVideo,
            const QAbstractItemModel *m, int row, const QModelIndex &parent)
{
    if (!fileView || (exts.isEmpty() && !adultVideo))
        return true;
    const QString name = cell(m, row, COLUMN_FINISHED_NAME, parent);
    const QString path = cell(m, row, COLUMN_FINISHED_PATH, parent);
    QString candidate = cell(m, row, COLUMN_FINISHED_TARGET, parent);
    if (candidate.isEmpty())
        candidate = path + name;
    const QString fileName = name.isEmpty() ? QFileInfo(candidate).fileName() : name;
    const QString dir = path.isEmpty() ? candidate : path;
    return SearchFileTypes::matchesFile(fileName, dir, exts, adultVideo);
}

bool textOk(const QString &text, const QAbstractItemModel *m, int row, const QModelIndex &parent)
{
    if (text.isEmpty())
        return true;
    const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
    return cell(m, row, COLUMN_FINISHED_NAME, parent).contains(text, cs)
            || cell(m, row, COLUMN_FINISHED_PATH, parent).contains(text, cs)
            || cell(m, row, COLUMN_FINISHED_USER, parent).contains(text, cs)
            || cell(m, row, COLUMN_FINISHED_TARGET, parent).contains(text, cs);
}

} // namespace

bool isFinishedFileList(const string &path) {
    if (path.empty())
        return false;

    const string listPath = Util::getListPath();
    if (!listPath.empty() && path.size() >= listPath.size() &&
        Util::stricmp(path.substr(0, listPath.size()).c_str(), listPath.c_str()) == 0)
        return true;

    if (path.size() >= 4 && Util::stricmp(path.substr(path.size() - 4).c_str(), ".xml") == 0)
        return true;
    if (path.size() >= 7 && Util::stricmp(path.substr(path.size() - 7).c_str(), ".xml.bz2") == 0)
        return true;

    return Util::stricmp(Util::getFileExt(path).c_str(), ".DcLst") == 0;
}

void FinishedTransferProxyModel::setFullOnly(bool fullOnly) {
    if (fullOnly_ == fullOnly)
        return;
    fullOnly_ = fullOnly;
    WULFOR_INVALIDATE_FILTER();
}

void FinishedTransferProxyModel::setTextFilter(const QString &text) {
    if (textFilter_ == text)
        return;
    textFilter_ = text;
    WULFOR_INVALIDATE_FILTER();
}

void FinishedTransferProxyModel::setFileView(bool fileView) {
    if (fileView_ == fileView)
        return;
    fileView_ = fileView;
    WULFOR_INVALIDATE_FILTER();
}

void FinishedTransferProxyModel::setTypeFilter(const QStringList &exts, bool adultVideo) {
    if (extFilter_ == exts && adultVideo_ == adultVideo)
        return;
    extFilter_ = exts;
    adultVideo_ = adultVideo;
    WULFOR_INVALIDATE_FILTER();
}

bool FinishedTransferProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    const QAbstractItemModel *model = sourceModel();
    if (!model)
        return true;
    if (!fileListOk(hideFileLists_, requireFullFile_, model, sourceRow, sourceParent))
        return false;
    if (fullOnly_ && !rowIsFull(model, sourceRow, sourceParent))
        return false;
    return typeOk(fileView_, extFilter_, adultVideo_, model, sourceRow, sourceParent)
            && textOk(textFilter_, model, sourceRow, sourceParent);
}
