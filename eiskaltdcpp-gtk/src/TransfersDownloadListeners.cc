/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "transfers.hh"
#include "TransfersOpen.hh"
#include "wulformanager.hh"
#include "func.hh"

#include <dcpp/Download.h>
#include <dcpp/QueueManager.h>
#include <dcpp/TimerManager.h>
#include <dcpp/Util.h>

using namespace std;
using namespace dcpp;

void Transfers::on(DownloadManagerListener::Requesting, Download* dl) noexcept
{
    StringMap params;

    getParams_client(params, dl);

    params["File Size"] = Util::toString(QueueManager::getInstance()->getSize(dl->getPath()));
    params["File Position"] = Util::toString(QueueManager::getInstance()->getPos(dl->getPath()));
    params["Sort Order"] = "w" + params["User"];
    params["Status"] = _("Requesting");
    params["Failed"] = "0";

    typedef Func1<Transfers, StringMap> F1;
    F1* f1 = new F1(this, &Transfers::initTransfer_gui, params);
    WulforManager::get()->dispatchGuiFunc(f1);
}

void Transfers::on(DownloadManagerListener::Queued, Download* dl, size_t queuePos) noexcept
{
    StringMap params;

    getParams_client(params, dl);
    params["Status"] = queuePos > 0 ?
            str(F_("Queue position %1%") % queuePos) :
            _("Waiting in upload queue");
    params["Sort Order"] = "w" + params["User"];
    params["Failed"] = "0";

    typedef Func3<Transfers, StringMap, bool, Sound::TypeSound> F3;
    F3* f3 = new F3(this, &Transfers::updateTransfer_gui, params, false, Sound::NONE);
    WulforManager::get()->dispatchGuiFunc(f3);
}

void Transfers::on(DownloadManagerListener::Starting, Download* dl) noexcept
{
    StringMap params;

    getParams_client(params, dl);
    params["Status"] = _("Download starting...");
    params["Sort Order"] = "d" + params["User"];
    params["Failed"] = "0";
    params["tmpTarget"] = dl->getTempTarget();

    typedef Func3<Transfers, StringMap, bool, Sound::TypeSound> F3;
    F3* f3 = new F3(this, &Transfers::updateTransfer_gui, params, true, Sound::NONE);
    WulforManager::get()->dispatchGuiFunc(f3);
}

void Transfers::on(DownloadManagerListener::Tick, const DownloadList& dls) noexcept
{
    for (auto& it : dls)
    {
        Download* dl = it;
        StringMap params;
        string flags;

        getParams_client(params, dl);

        if (dl->getUserConnection().isSecure())
        {
            if (dl->getUserConnection().isTrusted())
                flags += _("[S]");
            else
                flags += _("[U]");
        }
        if (dl->isSet(Download::FLAG_TTH_CHECK))
            flags += _("[T]");
        if (dl->isSet(Download::FLAG_ZDOWNLOAD))
            flags += _("[Z]");

        params["Flags"] = flags;
        params["Transferred"] = Util::formatBytes(dl->getPos());
        params["Time"] = Util::formatSeconds((GET_TICK() - dl->getStart()) / 1000);
        params["Progress"] = params["Progress Hidden"] + "%";
        params["Status"] = _("Downloading");

        typedef Func3<Transfers, StringMap, bool, Sound::TypeSound> F3;
        F3* f3 = new F3(this, &Transfers::updateTransfer_gui, params, true, Sound::NONE);
        WulforManager::get()->dispatchGuiFunc(f3);
    }
}

void Transfers::on(DownloadManagerListener::Complete, Download* dl) noexcept
{
    StringMap params;

    getParams_client(params, dl);
    params["Status"] = _("Download complete...");
    params["Progress Hidden"] = "100";
    params["Progress"] = "100%";
    params["Sort Order"] = "w" + params["User"];
    params["Speed"] = "-1";

    int64_t pos = QueueManager::getInstance()->getPos(dl->getPath()) + dl->getPos();

    typedef Func3<Transfers, StringMap, bool, Sound::TypeSound> F3;
    F3* f3 = new F3(this, &Transfers::updateTransfer_gui, params, true, Sound::NONE);
    WulforManager::get()->dispatchGuiFunc(f3);

    typedef Func2<Transfers, const string, int64_t> F2b;
    F2b* f2b = new F2b(this, &Transfers::updateFilePosition_gui, params["CID"], pos);
    WulforManager::get()->dispatchGuiFunc(f2b);
}

void Transfers::on(DownloadManagerListener::Failed, Download* dl, const string& reason) noexcept
{
    onFailed(dl, reason);
}

void Transfers::on(QueueManagerListener::CRCFailed, Download* dl, const string& reason) noexcept
{
    onFailed(dl, reason);
}

void Transfers::onFailed(Download* dl, const string& reason) {
    StringMap params;
    getParams_client(params, dl);
    params["Status"] = TransfersOpen::stripBracketedStatusPrefix(reason);
    params["Sort Order"] = "w" + params["User"];
    params["Failed"] = "1";
    params["Speed"] = "-1";
    params["Time Left"] = "-1";

    int64_t pos = QueueManager::getInstance()->getPos(dl->getPath()) + dl->getPos();

    typedef Func3<Transfers, StringMap, bool, Sound::TypeSound> F3;
    F3* f3 = new F3(this, &Transfers::updateTransfer_gui, params, true, Sound::NONE);
    WulforManager::get()->dispatchGuiFunc(f3);

    typedef Func2<Transfers, const string, int64_t> F2b;
    F2b* f2b = new F2b(this, &Transfers::updateFilePosition_gui, params["CID"], pos);
    WulforManager::get()->dispatchGuiFunc(f2b);
}
