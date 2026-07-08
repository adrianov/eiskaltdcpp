/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, compiling, linking, and/or
 * using OpenSSL with this program is allowed.
 */

#include "finishedtransfers.hh"
#include <dcpp/ClientManager.h>
#include "wulformanager.hh"
#include "WulforUtil.hh"

using namespace std;
using namespace dcpp;

void FinishedTransfers::initializeList_client()
{
    StringMap params;
    auto lock = FinishedManager::getInstance()->lock();
    const FinishedManager::MapByFile &list = FinishedManager::getInstance()->getMapByFile(isUpload);
    const FinishedManager::MapByUser &user = FinishedManager::getInstance()->getMapByUser(isUpload);

    for (auto it = list.begin(); it != list.end(); ++it)
    {
        params.clear();
        getFinishedParams_client(it->second, it->first, params);
        addFile_gui(params, false);
    }

    for (auto uit = user.begin(); uit != user.end(); ++uit)
    {
        params.clear();
        getFinishedParams_client(uit->second, uit->first, params);
        addUser_gui(params, false);
    }

    updateStatus_gui();
    resortStore(fileStore);
    resortStore(userStore);
}

void FinishedTransfers::getFinishedParams_client(const FinishedFileItemPtr& item, const string& file, StringMap &params)
{
    string nicks;
    params["Filename"] = Util::getFileName(file);
    params["Time"] = Util::formatTime("%Y-%m-%d %H:%M:%S", item->getTime());
    params["Path"] = Util::getFilePath(file);
    for (auto &user : item->getUsers())
    {
        nicks += WulforUtil::getNicks(user.user->getCID(), user.hint) + ", ";
    }
    params["Nicks"] = nicks.substr(0, nicks.length() - 2);
    params["Transferred"] = Util::toString(item->getTransferred());
    params["Speed"] = Util::toString(item->getAverageSpeed());
    params["CRC Checked"] = item->getCrc32Checked() ? _("Yes") : _("No");
    params["Target"] = file;
    params["Elapsed Time"] = Util::toString(item->getMilliSeconds());

    if (item->isFull())
        params["full"] = "1";
    else
        params["full"] = "0";
}

void FinishedTransfers::getFinishedParams_client(const FinishedUserItemPtr &item, const HintedUser &user, StringMap &params)
{
    string files;
    params["Time"] = Util::formatTime("%Y-%m-%d %H:%M:%S", item->getTime());
    params["Nick"] = WulforUtil::getNicks(user);
    params["Hub"] = WulforUtil::getHubNames(user);
    for (auto &file : item->getFiles())
    {
        files += file + ", ";
    }
    params["Files"] = files.substr(0, files.length() - 2);
    params["Transferred"] = Util::toString(item->getTransferred());
    params["Speed"] = Util::toString(item->getAverageSpeed());
    params["CID"] = user.user->getCID().toBase32();
    params["Elapsed Time"] = Util::toString(item->getMilliSeconds());
}

void FinishedTransfers::removeUser_client(string cid)
{
    UserPtr user = ClientManager::getInstance()->findUser(CID(cid));

    if (user)
        FinishedManager::getInstance()->remove(isUpload, HintedUser(user, ""));
}

void FinishedTransfers::removeFile_client(string target)
{
    if (!target.empty())
        FinishedManager::getInstance()->remove(isUpload, target);
}

void FinishedTransfers::removeAll_client()
{
    FinishedManager::getInstance()->removeAll(isUpload);
}

void FinishedTransfers::pruneMissingFiles_client()
{
    StringList missing;
    {
        auto lock = FinishedManager::getInstance()->lock();
        const FinishedManager::MapByFile &list = FinishedManager::getInstance()->getMapByFile(false);
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (!Util::fileExists(it->first))
                missing.push_back(it->first);
        }
    }

    for (const auto &file : missing)
        removeFile_client(file);
}

void FinishedTransfers::on(FinishedManagerListener::AddedFile, bool upload, const string& file, const FinishedFileItemPtr& item) noexcept
{
    if (isUpload == upload)
    {
        StringMap params;
        getFinishedParams_client(item, file, params);

        typedef Func2<FinishedTransfers, StringMap, bool> F2;
        F2* func = new F2(this, &FinishedTransfers::addFile_gui, params, true);
        WulforManager::get()->dispatchGuiFunc(func);
    }
}

void FinishedTransfers::on(FinishedManagerListener::AddedUser, bool upload, const HintedUser &user, const FinishedUserItemPtr &item) noexcept
{
    if (isUpload == upload)
    {
        StringMap params;
        getFinishedParams_client(item, user, params);

        typedef Func2<FinishedTransfers, StringMap, bool> F2;
        F2 *func = new F2(this, &FinishedTransfers::addUser_gui, params, true);
        WulforManager::get()->dispatchGuiFunc(func);
    }
}

void FinishedTransfers::on(FinishedManagerListener::UpdatedFile, bool upload, const string& file, const FinishedFileItemPtr& item) noexcept
{
    if (isUpload == upload)
    {
        StringMap params;
        getFinishedParams_client(item, file, params);

        typedef Func2<FinishedTransfers, StringMap, bool> F2;
        F2 *func = new F2(this, &FinishedTransfers::addFile_gui, params, true);
        WulforManager::get()->dispatchGuiFunc(func);
    }
}

void FinishedTransfers::on(FinishedManagerListener::UpdatedUser, bool upload, const HintedUser &user) noexcept
{
    if (isUpload == upload)
    {
        const FinishedManager::MapByUser &umap = FinishedManager::getInstance()->getMapByUser(isUpload);
        auto userit = umap.find(user);
        if (userit == umap.end())
            return;

        const FinishedUserItemPtr &item = userit->second;

        StringMap params;
        getFinishedParams_client(item, user, params);

        typedef Func2<FinishedTransfers, StringMap, bool> F2;
        F2 *func = new F2(this, &FinishedTransfers::addUser_gui, params, true);
        WulforManager::get()->dispatchGuiFunc(func);
    }
}

void FinishedTransfers::on(FinishedManagerListener::RemovedFile, bool upload, const string& item) noexcept
{
    if (isUpload == upload)
    {
        typedef Func1<FinishedTransfers, string> F1;
        F1 *func = new F1(this, &FinishedTransfers::removeFile_gui, item);
        WulforManager::get()->dispatchGuiFunc(func);
    }
}

void FinishedTransfers::on(FinishedManagerListener::RemovedUser, bool upload, const HintedUser &user) noexcept
{
    if (isUpload == upload)
    {
        typedef Func1<FinishedTransfers, string> F1;
        F1 *func = new F1(this, &FinishedTransfers::removeUser_gui, user.user->getCID().toBase32());
        WulforManager::get()->dispatchGuiFunc(func);
    }
}
