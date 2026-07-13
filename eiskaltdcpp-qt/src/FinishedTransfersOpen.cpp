/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfers.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

#if defined(Q_OS_MAC)
#include <CoreServices/CoreServices.h>
#include <QFile>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <shellapi.h>
#elif defined(Q_OS_UNIX)
#include <QFile>
#include <QMimeDatabase>
#include <QSet>
#include <QStandardPaths>
#endif

namespace {

QString resolveLocalFile(QString file)
{
#if !defined(Q_OS_WIN)
    // Legacy targets may omit the leading slash; keep them rooted.
    if (!file.isEmpty() && !QFileInfo(file).isAbsolute())
        file.prepend(QLatin1Char('/'));
#endif

    QFileInfo info(file);
    if (info.exists())
        return info.absoluteFilePath();

    const QString name = info.fileName();
    QDir dir(info.absolutePath());
    if (name.isEmpty() || !dir.exists())
        return info.absoluteFilePath();

    const QStringList matches = dir.entryList(QStringList(QStringLiteral("*%1*").arg(name)),
                                              QDir::Files, QDir::Name);
    if (!matches.isEmpty())
        return dir.absoluteFilePath(matches.first());
    return info.absoluteFilePath();
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
void absorbMimeApps(const QString &path, QSet<QString> &mimes)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    bool in = false;
    while (!f.atEnd()) {
        const QByteArray line = f.readLine().trimmed();
        if (line.startsWith('[')) {
            in = line == "[MIME Cache]" || line == "[Default Applications]"
                 || line == "[Added Associations]";
            continue;
        }
        if (!in || line.isEmpty() || line.startsWith('#'))
            continue;
        const int eq = line.indexOf('=');
        if (eq <= 0)
            continue;
        const QString mime = QString::fromUtf8(line.left(eq).trimmed());
        if (line.mid(eq + 1).trimmed().isEmpty())
            mimes.remove(mime);
        else
            mimes.insert(mime);
    }
}

bool unixMimeHasApp(const QFileInfo &info)
{
    static QSet<QString> mimesWithApp;
    static bool ready = false;
    static bool present = false;
    if (!ready) {
        ready = true;
        const QStringList roots = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
        for (const QString &root : roots)
            absorbMimeApps(root + QStringLiteral("/applications/mimeinfo.cache"), mimesWithApp);
        for (const QString &root : roots)
            absorbMimeApps(root + QStringLiteral("/applications/mimeapps.list"), mimesWithApp);
        absorbMimeApps(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                           + QStringLiteral("/mimeapps.list"),
                       mimesWithApp);
        present = !mimesWithApp.isEmpty();
    }
    if (!present)
        return true; // keep legacy open when desktop MIME DB is unavailable
    static QMimeDatabase db;
    return mimesWithApp.contains(db.mimeTypeForFile(info).name());
}
#endif

bool hasAssociatedApp(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists())
        return false;

#if defined(Q_OS_MAC)
    const QByteArray utf8 = QFile::encodeName(info.absoluteFilePath());
    CFURLRef fileUrl = CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8 *>(utf8.constData()),
        utf8.size(),
        info.isDir());
    if (!fileUrl)
        return false;

    CFURLRef appUrl = LSCopyDefaultApplicationURLForURL(fileUrl, kLSRolesAll, nullptr);
    CFRelease(fileUrl);
    if (!appUrl)
        return false;
    CFRelease(appUrl);
    return true;
#elif defined(Q_OS_WIN)
    wchar_t exe[MAX_PATH] = {};
    const HINSTANCE result = FindExecutableW(
        reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(info.absoluteFilePath()).utf16()),
        nullptr,
        exe);
    return reinterpret_cast<INT_PTR>(result) > 32;
#elif defined(Q_OS_UNIX)
    return unixMimeHasApp(info);
#else
    return true;
#endif
}

} // namespace

template <bool isUpload>
void FinishedTransfers<isUpload>::openFile(QString file)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(resolveLocalFile(file)));
}

template <bool isUpload>
void FinishedTransfers<isUpload>::openOrReveal(QString file)
{
    file = resolveLocalFile(file);
    if (hasAssociatedApp(file)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(file));
        return;
    }

    // No handler (or missing file): show in file manager. open -R fails if gone.
    const QFileInfo info(file);
    WulforUtil::revealPath(info.exists() ? file : info.absolutePath());
}

template void FinishedTransfers<true>::openFile(QString);
template void FinishedTransfers<false>::openFile(QString);
template void FinishedTransfers<true>::openOrReveal(QString);
template void FinishedTransfers<false>::openOrReveal(QString);
