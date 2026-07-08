/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "TransfersOpen.hh"

#include <glib/gi18n.h>

#include <dcpp/File.h>
#include <dcpp/QueueManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/Util.h>

using namespace std;
using namespace dcpp;

namespace TransfersOpen {

bool isOpenableTransfer(const string& filename) {
    return !filename.empty()
        && filename != _("File list")
        && filename.find(_("TTH: ")) != 0;
}

string resolveUploadPath(const string& target) {
    try {
        const StringList paths = ShareManager::getInstance()->getRealPaths(Util::toAdcFile(target));
        if (!paths.empty() && File::getSize(paths.front()) > -1)
            return paths.front();
    } catch (...) {}
    return string();
}

string resolveDownloadPath(const string& target, const string& tmpTarget) {
    if (!tmpTarget.empty() && tmpTarget != _("none") && File::getSize(tmpTarget) > -1)
        return tmpTarget;
    string tempPath;
    string finalPath;
    string pathStr = target;
    {
        const QueueItem::StringMap &ll = QueueManager::getInstance()->lockQueue();
        const auto it = ll.find(const_cast<string*>(&pathStr));
        if (it != ll.end()) {
            tempPath = it->second->getTempTarget();
            finalPath = it->second->getTarget();
        }
        QueueManager::getInstance()->unlockQueue();
    }
    if (!tempPath.empty() && File::getSize(tempPath) > -1)
        return tempPath;
    if (!finalPath.empty() && File::getSize(finalPath) > -1)
        return finalPath;
    if (!target.empty() && File::getSize(target) > -1)
        return target;
    return string();
}

string resolveTransferPath(bool download, const string& target, const string& tmpTarget, const string& filename) {
    if (!isOpenableTransfer(filename))
        return string();
    return download ? resolveDownloadPath(target, tmpTarget) : resolveUploadPath(target);
}

string stripBracketedStatusPrefix(const string& text) {
    if (text.size() < 4 || text[0] != '<')
        return text;
    const size_t sep = text.find(">: ");
    return sep != string::npos && sep > 0 ? text.substr(sep + 3) : text;
}

} // namespace TransfersOpen
