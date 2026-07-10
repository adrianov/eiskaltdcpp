/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfers.h"

#include "dcpp/File.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/ShareManager.h"

#include <QFileInfo>
#include <QSet>

namespace {

int dirDepth(const QString &path)
{
    return QDir::fromNativeSeparators(QDir::cleanPath(path))
        .split(QLatin1Char('/'), WULFOR_SKIP_EMPTY).size();
}

void removeEmptyParents(QString dir)
{
    const QString downloadDir = QDir::cleanPath(_q(SETTING(DOWNLOAD_DIRECTORY)));
    const QString root = QDir::cleanPath(QDir::rootPath());

    while (!dir.isEmpty()) {
        dir = QDir::cleanPath(dir);
        if (dir == root || dir == downloadDir)
            break;
        if (dirDepth(dir) < 2)
            break;

        QDir d(dir);
        if (!d.exists())
            break;
        if (!d.entryList(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System).isEmpty())
            break;
        if (!QDir().rmdir(dir))
            break;

        dir = QFileInfo(dir).absolutePath();
    }
}

} // namespace

template <bool isUpload>
void FinishedTransfers<isUpload>::pruneMissingFiles()
{
    const QStringList targets = model->fileTargets();
    for (const QString &qtarget : targets) {
        if (Util::fileExists(_tq(qtarget)))
            continue;

        try {
            FinishedManager::getInstance()->remove(false, _tq(qtarget));
        } catch (const std::exception&) {}

        removeFileFromDB(qtarget);
        model->remFile(qtarget);
    }
}

template <bool isUpload>
void FinishedTransfers<isUpload>::deleteDiskFiles(const QStringList &files)
{
    QSet<QString> parents;
    for (const QString &f : files) {
        if (f.isEmpty())
            continue;
        try {
            const string path = _tq(f);
            ShareManager::getInstance()->removeFile(path);
            File::deleteFile(path);
            FinishedManager::getInstance()->remove(isUpload, path);
        } catch (const std::exception&) {}

        removeFileFromDB(f);
        model->remFile(f);
        parents.insert(QFileInfo(f).absolutePath());
    }
    for (const QString &dir : parents)
        removeEmptyParents(dir);
}

template void FinishedTransfers<true>::pruneMissingFiles();
template void FinishedTransfers<false>::pruneMissingFiles();
template void FinishedTransfers<true>::deleteDiskFiles(const QStringList&);
template void FinishedTransfers<false>::deleteDiskFiles(const QStringList&);
