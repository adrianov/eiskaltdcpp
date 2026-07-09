/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "MainAppCli.h"

#include "MainWindow.h"
#include "ShareIndex.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/DCPlusPlus.h"
#include "dcpp/Util.h"
#include "dcpp/SettingsManager.h"

#include "dcpp/DirectoryListing.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <cstdlib>
#include <iostream>

using namespace dcpp;

#ifdef USE_QT_SQLITE
static int runShareIndexReingest(const QString &listPath)
{
    if (listPath.isEmpty() || !QFileInfo::exists(listPath)) {
        std::cerr << "Usage: --share-index-reingest <path-to-xml.bz2>" << std::endl;
        return 1;
    }

    startup(nullptr, nullptr);
    SettingsManager::getInstance()->set(SettingsManager::KEEP_LISTS, true);

    const UserPtr user = DirectoryListing::getUserFromFilename(_tq(listPath));
    if (!user) {
        std::cerr << "Cannot parse CID from list filename" << std::endl;
        return 1;
    }

    QString nick;
    const QString base = QFileInfo(listPath).completeBaseName(); // strips .bz2; still has .xml
    QString stem = QFileInfo(listPath).fileName();
    if (stem.endsWith(QLatin1String(".xml.bz2"), Qt::CaseInsensitive))
        stem.chop(8);
    else if (stem.endsWith(QLatin1String(".xml"), Qt::CaseInsensitive))
        stem.chop(4);
    const int dot = stem.lastIndexOf(QLatin1Char('.'));
    if (dot > 0)
        nick = stem.left(dot);

    ShareIndex::newInstance();
    ShareIndex::getInstance()->open();

    const QString cid = _q(user->getCID().toBase32());
    std::cout << "Force reingest cid=" << cid.toStdString()
              << " file=" << QFileInfo(listPath).fileName().toStdString()
              << " size=" << QFileInfo(listPath).size() << " bytes" << std::endl;

    const qint64 ms = ShareIndex::getInstance()->forceIngestListMs(user, listPath, QString(), nick);
    if (!ShareIndex::getInstance()->lastError().isEmpty())
        std::cerr << "Ingest error: " << ShareIndex::getInstance()->lastError().toStdString() << std::endl;

    std::cout << "Reingest wall time: " << ms << " ms ("
              << (ms / 1000.0) << " s)" << std::endl;

    ShareIndex::deleteInstance();
    Q_UNUSED(base);
    return 0;
}

static int runShareIndexIngest(const QStringList &terms)
{
    startup(nullptr, nullptr);
    // Avoid QueueManager dtor wiping FileLists (KeepLists defaults off).
    SettingsManager::getInstance()->set(SettingsManager::KEEP_LISTS, true);

    ShareIndex::newInstance();
    ShareIndex::getInstance()->open();
    ShareIndex::getInstance()->ingestCachedLists();
    ShareIndex::getInstance()->waitWritesIdle();
    if (!ShareIndex::getInstance()->lastError().isEmpty())
        std::cerr << "Ingest error: " << ShareIndex::getInstance()->lastError().toStdString() << std::endl;

    qint64 rows = 0;
    const QString dbPath = QString::fromStdString(Util::getPath(Util::PATH_USER_CONFIG))
            + "ShareIndex.sqlite";
    {
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ShareIndexCount");
            db.setDatabaseName(dbPath);
            if (db.open()) {
                QSqlQuery q(db);
                if (q.exec("SELECT count(*) FROM share_entries") && q.next())
                    rows = q.value(0).toLongLong();
                db.close();
            }
        }
        QSqlDatabase::removeDatabase("ShareIndexCount");
    }

    std::cout << "ShareIndex rows: " << rows << std::endl;
    if (!terms.isEmpty()) {
        ShareIndex::SearchFilter f;
        f.terms = terms;
        f.limit = 20;
        const auto hits = ShareIndex::getInstance()->search(f);
        std::cout << "Search hits: " << hits.size() << std::endl;
        for (const auto &h : hits) {
            std::cout << "  " << h.value("FILE").toString().toStdString()
                      << " [" << h.value("NICK").toString().toStdString() << "]"
                      << std::endl;
        }
    }

    ShareIndex::deleteInstance();
    // Skip dcpp::shutdown() so FileLists are not deleted on exit.
    return 0;
}
#endif

void parseCmdLine(const QStringList &args){
    for (int i = 0; i < args.size(); ++i) {
        const QString &arg = args.at(i);
        if (arg == "-h" || arg == "--help"){
            About().printHelp();
            exit(0);
        }
        else if (arg == "-V" || arg == "--version"){
            About().printVersion();
            exit(0);
        }
#ifdef USE_QT_SQLITE
        else if (arg == "--share-index-smoke"){
            QString err;
            if (!ShareIndex::smokeCheck(&err)) {
                std::cerr << "ShareIndex smoke failed: " << err.toStdString() << std::endl;
                exit(1);
            }
            std::cout << "ShareIndex smoke OK" << std::endl;
            exit(0);
        }
        else if (arg == "--share-index-ingest"){
            QStringList terms;
            for (int j = i + 1; j < args.size(); ++j)
                terms << args.at(j);
            exit(runShareIndexIngest(terms));
        }
        else if (arg == "--share-index-reingest"){
            const QString path = (i + 1 < args.size()) ? args.at(i + 1) : QString();
            exit(runShareIndexReingest(path));
        }
#endif
    }
}
