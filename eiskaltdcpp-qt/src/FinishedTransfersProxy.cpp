/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfersProxy.h"
#include "FinishedTransfersModel.h"

#include "dcpp/stdinc.h"
#include "dcpp/Util.h"
#include "WulforUtil.h"

using namespace dcpp;

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
    invalidateFilter();
}

void FinishedTransferProxyModel::setTextFilter(const QString &text) {
    if (textFilter_ == text)
        return;
    textFilter_ = text;
    invalidateFilter();
}

bool FinishedTransferProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    const QAbstractItemModel *model = sourceModel();
    if (!model)
        return true;

    const QModelIndex nameIndex = model->index(sourceRow, COLUMN_FINISHED_NAME, sourceParent);
    const QModelIndex pathIndex = model->index(sourceRow, COLUMN_FINISHED_PATH, sourceParent);
    const QModelIndex userIndex = model->index(sourceRow, COLUMN_FINISHED_USER, sourceParent);
    const QModelIndex targetIndex = model->index(sourceRow, COLUMN_FINISHED_TARGET, sourceParent);
    const QModelIndex fullIndex = model->index(sourceRow, COLUMN_FINISHED_FULL, sourceParent);

    if (hideFileLists_ || requireFullFile_) {
        const string target = _tq(model->data(targetIndex, Qt::DisplayRole).toString());
        const string path = _tq(model->data(pathIndex, Qt::DisplayRole).toString() +
                                model->data(nameIndex, Qt::DisplayRole).toString());

        if (hideFileLists_ && (isFinishedFileList(target) || isFinishedFileList(path)))
            return false;

        if (requireFullFile_ && model->data(fullIndex, Qt::DisplayRole).toString() != QLatin1String("1"))
            return false;
    }

    if (fullOnly_ && model->data(fullIndex, Qt::DisplayRole).toString() != QLatin1String("1"))
        return false;

    if (!textFilter_.isEmpty()) {
        const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
        const QString name = model->data(nameIndex, Qt::DisplayRole).toString();
        const QString path = model->data(pathIndex, Qt::DisplayRole).toString();
        const QString user = model->data(userIndex, Qt::DisplayRole).toString();
        const QString target = model->data(targetIndex, Qt::DisplayRole).toString();
        if (!name.contains(textFilter_, cs) && !path.contains(textFilter_, cs) &&
            !user.contains(textFilter_, cs) && !target.contains(textFilter_, cs))
            return false;
    }

    return true;
}
