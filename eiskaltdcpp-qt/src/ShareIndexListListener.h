/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "dcpp/stdinc.h"
#include "dcpp/QueueManager.h"
#include "dcpp/Singleton.h"

#include <QObject>

/** QueueManager hooks that feed ShareIndex and open ShareBrowser. */
class ShareIndexListListener :
        public QObject,
        public dcpp::Singleton<ShareIndexListListener>,
        private dcpp::QueueManagerListener
{
    Q_OBJECT
    friend class dcpp::Singleton<ShareIndexListListener>;

public:
    void start();
    void stop();

Q_SIGNALS:
    void openShare(dcpp::UserPtr user, const QString &listPath, const QString &dir);
    void queueEmpty();

private:
    ShareIndexListListener() = default;
    ~ShareIndexListListener() override;

    void on(dcpp::QueueManagerListener::Finished, dcpp::QueueItem*,
            const std::string&, int64_t) noexcept override;
    void on(dcpp::QueueManagerListener::ListFromCache, const dcpp::HintedUser&,
            const std::string& listPath, const std::string& initialDir) noexcept override;
    void on(dcpp::QueueManagerListener::ListCached, const dcpp::HintedUser&,
            const std::string& listPath) noexcept override;
    void on(dcpp::QueueManagerListener::SourceRemoved, dcpp::QueueItem*,
            const dcpp::UserPtr&, int reason) noexcept override;
    void on(dcpp::QueueManagerListener::PeerUnreachable, const dcpp::UserPtr&) noexcept override;
};
