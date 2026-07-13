/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchLocalPath.h"
#include "WulforUtil.h"

#include "dcpp/File.h"
#include "dcpp/FinishedManager.h"
#include "dcpp/QueueManager.h"
#include "dcpp/ShareManager.h"

#include <QDesktopServices>
#include <QUrl>

using namespace dcpp;

namespace SearchLocalPath {

static QString fromShare(const QString &tth)
{
    if (tth.isEmpty())
        return QString();

    ShareManager *sm = ShareManager::getInstance();
    try {
        const string path = sm->toReal(sm->toVirtual(TTHValue(_tq(tth))));
        if (File::getSize(path) > -1)
            return _q(path);
    } catch (...) {}

    return QString();
}

static QString fromFinished(const QString &tth, qint64 size)
{
    // size <= 0 is not authoritative (missing ESIZE / dirs); avoid matching a 0-byte file.
    if (tth.isEmpty() || size <= 0)
        return QString();

    const string path = FinishedManager::getInstance()->getTarget(_tq(tth));
    if (path.empty() || File::getSize(path) != size)
        return QString();
    return _q(path);
}

QString resolve(const QString &tth, qint64 size)
{
    const QString shared = fromShare(tth);
    if (!shared.isEmpty())
        return shared;
    return fromFinished(tth, size);
}

bool isQueued(const QString &tth)
{
    if (tth.isEmpty())
        return false;
    // Same TTH index DONT_DL_ALREADY_QUEUED uses; empty-TTH filelists never reach here.
    return !QueueManager::getInstance()->getTargets(TTHValue(_tq(tth))).empty();
}

bool openFile(const QString &path)
{
    if (path.isEmpty())
        return false;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

bool openDirectory(const QString &path)
{
    if (path.isEmpty())
        return false;
    return WulforUtil::revealPath(path);
}

} // namespace SearchLocalPath
