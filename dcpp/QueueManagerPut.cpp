/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
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
 */

#include "stdinc.h"
#include "QueueManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "Download.h"
#include "HashManager.h"
#include "ListCache.h"
#include "SettingsManager.h"
#include "Transfer.h"
#include "UserConnection.h"

namespace dcpp {

void QueueManager::putDownload(Download* aDownload, bool finished) noexcept {
    HintedUserList getConn;
    string fl_fname;
    HintedUser fl_user(UserPtr(), Util::emptyString);
    int fl_flag = 0;

    unique_ptr<Download> d(aDownload);
    aDownload = nullptr;

    {
        Lock l(cs);

        delete d->getFile();
        d->setFile(0);

        if(d->getType() == Transfer::TYPE_PARTIAL_LIST) {
            QueueItem* q = nullptr;
            try {
                q = fileQueue.find(getListPath(d->getHintedUser()));
            }
            catch(const Exception&) { }
            if(q) {
                if(!d->getPFS().empty()) {
                    if( (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) && directories.find(d->getUser()) != directories.end()) ||
                            (q->isSet(QueueItem::FLAG_MATCH_QUEUE)) )
                    {
                        dcassert(finished);

                        fl_fname = d->getPFS();
                        fl_user = d->getHintedUser();
                        fl_flag = (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) ? (QueueItem::FLAG_DIRECTORY_DOWNLOAD) : 0)
                                | (q->isSet(QueueItem::FLAG_MATCH_QUEUE) ? QueueItem::FLAG_MATCH_QUEUE : 0) | QueueItem::FLAG_TEXT;
                    } else {
                        fire(QueueManagerListener::PartialList(), d->getHintedUser(), d->getPFS());
                    }
                } else {
                    dcassert(!finished);
                    fl_flag = q->getFlags() & ~QueueItem::FLAG_PARTIAL_LIST;
                }
                fire(QueueManagerListener::Removed(), q);

                userQueue.remove(q);
                fileQueue.remove(q);
            }
        } else {
            QueueItem* q = fileQueue.find(d->getPath());

            if(q) {
                if(d->getType() == Transfer::TYPE_FULL_LIST) {
                    if(d->isSet(Download::FLAG_XML_BZ_LIST)) {
                        q->setFlag(QueueItem::FLAG_XML_BZLIST);
                    } else {
                        q->unsetFlag(QueueItem::FLAG_XML_BZLIST);
                    }
                }

                if(finished) {
                    if(d->getType() == Transfer::TYPE_TREE) {
                        dcassert(d->getTreeValid());
                        HashManager::getInstance()->addTree(d->getTigerTree());

                        userQueue.removeDownload(q, d->getUser());
                        fire(QueueManagerListener::StatusUpdated(), q);
                    } else {
                        if( (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) && directories.find(d->getUser()) != directories.end()) ||
                                (q->isSet(QueueItem::FLAG_MATCH_QUEUE)) )
                        {
                            fl_fname = q->getListName();
                            fl_user = d->getHintedUser();
                            fl_flag = (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) ? QueueItem::FLAG_DIRECTORY_DOWNLOAD : 0)
                                    | (q->isSet(QueueItem::FLAG_MATCH_QUEUE) ? QueueItem::FLAG_MATCH_QUEUE : 0);
                        }

                        string dir;
                        bool crcError = false;
                        const bool wasEmpty = q->getDownloadedBytes() == 0;
                        if(d->getType() == Transfer::TYPE_FULL_LIST) {
                            dir = q->getTempTarget();
                            q->addSegment(Segment(0, q->getSize()));
                            ListCache::saveListMeta(d->getUser()->getCID(),
                                ClientManager::getInstance()->getBytesShared(d->getUser()),
                                File::getSize(q->getListName()));
                        } else if(d->getType() == Transfer::TYPE_FILE) {
                            q->addSegment(d->getSegment());
                            if(wasEmpty)
                                userQueue.promotePartial(q);
                        }

                        if (q->isFinished() && BOOLSETTING(SFV_CHECK)) {
                            crcError = checkSfv(q, d.get());
                        }

                        if(d->getType() != Transfer::TYPE_FILE || q->isFinished()) {
                            if( d->getType() == Transfer::TYPE_FILE && !d->getTempTarget().empty() && (Util::stricmp(d->getPath().c_str(), d->getTempTarget().c_str()) != 0) ) {
                                moveFile(d->getTempTarget(), d->getPath());
                            }
                            if (BOOLSETTING(LOG_FINISHED_DOWNLOADS) && d->getType() == Transfer::TYPE_FILE) {
                                logFinishedDownload(q, d.get(), crcError);
                            }

                            fire(QueueManagerListener::Finished(), q, dir, d->getAverageSpeed());

                            userQueue.remove(q);

                            if(!BOOLSETTING(KEEP_FINISHED_FILES) || d->getType() == Transfer::TYPE_FULL_LIST) {
                                fire(QueueManagerListener::Removed(), q);
                                fileQueue.remove(q);
                            } else {
                                fire(QueueManagerListener::StatusUpdated(), q);
                            }
                        } else {
                            userQueue.removeDownload(q, d->getUser());
                            fire(QueueManagerListener::StatusUpdated(), q);
                        }
                        setDirty();
                    }
                } else {
                    if(d->getType() != Transfer::TYPE_TREE) {
                        if(q->getDownloadedBytes() == 0) {
                            q->setTempTarget(Util::emptyString);
                        }
                        if(q->isSet(QueueItem::FLAG_USER_LIST)) {
                            File::deleteFile(q->getListName());
                        }
                        if(d->getType() == Transfer::TYPE_FILE) {
                            int64_t downloaded = d->getPos();
                            downloaded -= downloaded % d->getTigerTree().getBlockSize();

                            if(downloaded > 0) {
                                const bool wasEmpty = q->getDownloadedBytes() == 0;
                                q->addSegment(Segment(d->getStartPos(), downloaded));
                                if(wasEmpty)
                                    userQueue.promotePartial(q);
                                setDirty();
                            }
                        }
                    }

                    if(q->getPriority() != QueueItem::PAUSED) {
                        q->getOnlineUsers(getConn);
                    }

                    userQueue.removeDownload(q, d->getUser());
                    fire(QueueManagerListener::StatusUpdated(), q);
                }
            } else if(d->getType() != Transfer::TYPE_TREE) {
                if(!d->getTempTarget().empty() && (d->getType() == Transfer::TYPE_FULL_LIST || d->getTempTarget() != d->getPath())) {
                    File::deleteFile(d->getTempTarget());
                }
            }
        }
    }

    for(auto& i: getConn) {
        ConnectionManager::getInstance()->getDownloadConnection(i);
    }

    if(!fl_fname.empty()) {
        processList(fl_fname, fl_user, fl_flag);
    }
}

} // namespace dcpp
