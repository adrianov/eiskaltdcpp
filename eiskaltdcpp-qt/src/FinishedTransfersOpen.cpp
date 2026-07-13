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
#include <QHash>
#include <QMimeDatabase>
#include <QProcess>
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
    static QHash<QString, bool> mimeHasApp;
    const QString mime = QMimeDatabase().mimeTypeForFile(info).name();
    const auto cached = mimeHasApp.constFind(mime);
    if (cached != mimeHasApp.cend())
        return *cached;

    QProcess proc;
    proc.start(QStringLiteral("xdg-mime"), {QStringLiteral("query"), QStringLiteral("default"), mime});
    bool hasApp = true; // keep legacy open when detection is unavailable
    if (proc.waitForFinished(1500) && proc.exitStatus() == QProcess::NormalExit)
        hasApp = !QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed().isEmpty();
    mimeHasApp.insert(mime, hasApp);
    return hasApp;
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
