/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueManager.h"

#include "File.h"
#include "SettingsManager.h"
#include "ShareManager.h"

#if !defined(_WIN32) && !defined(PATH_MAX)
#if defined(__linux)
#include <sys/syslimits.h>
#elif defined(__GNU__)
#define PATH_MAX 4096
#endif
#endif

namespace dcpp {

void QueueManager::add(const string& aTarget, int64_t aSize, const TTHValue& root)
{
    // Check if we're not downloading something already in our share
    if (BOOLSETTING(DONT_DL_ALREADY_SHARED))
    {
        if (ShareManager::getInstance()->isTTHShared(root))
        {
            throw QueueException(_("A file with the same hash already exists in your share"));
        }
    }
    // Check that target contains at least one directory...we don't want headless files...
    // Check that the file doesn't already exist...
    const string target = checkTarget(aTarget, aSize);

    // Check if it's a zero-byte file, if so, create and return...
    // Hashed magnets with size 0 still queue (size may arrive from the peer).
    if (aSize == 0 && !root)
    {
        if (!BOOLSETTING(SKIP_ZERO_BYTE))
        {
            File::ensureDirectory(target);
            File f(target, File::WRITE, File::CREATE);
        }
        return;
    }

    Lock l(cs);

    // This will be pretty slow on large queues...
    if (BOOLSETTING(DONT_DL_ALREADY_QUEUED))
    {
        auto ql = fileQueue.find(root);
        if (!ql.empty())
            throw QueueException(_("This file is already queued"));
    }

    QueueItem* q = fileQueue.find(target);

    if(q == NULL)
    {
        q = fileQueue.add(target, aSize, 0, QueueItem::DEFAULT/*QueueItem::Priority*/, Util::emptyString/*aTempTarget*/,
                          GET_TIME()/*time_t aAdded*/, root/*TTHValue& root*/);
        fire(QueueManagerListener::Added(), q);
    } else {
        if(q->getSize() != aSize)
        {
            throw QueueException(_("A file with a different size already exists in the queue"));
        }
        if(!(root == q->getTTH()))
        {
            throw QueueException(_("A file with different tth root already exists in the queue"));
        }
    }
}
// NOTE: freedcpp: end

} // namespace dcpp
