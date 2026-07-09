/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "DownloadQueue.h"
#include "DownloadQueuePrivate.h"
#include "DownloadQueueModel.h"
#include "HubFrame.h"
#include "HubManager.h"

#include "dcpp/ClientManager.h"
#include "dcpp/User.h"
#include "dcpp/QueueManager.h"
#include "dcpp/Exception.h"

#include <QFileDialog>
#include <QDir>

using namespace dcpp;

void DownloadQueue::contextMoreActions(Menu::Action act, const QList<DownloadQueueItem*> &items,
                                       const QString &target, const QVariant &arg)
{
    Q_D(DownloadQueue);
    QueueManager *QM = QueueManager::getInstance();
    VarMap rmap;

    switch (act){
        case Menu::RenameMove:
        {
            for (const auto &i : items){
                QString itemTarget = i->data(COLUMN_DOWNLOADQUEUE_PATH).toString() +
                                 i->data(COLUMN_DOWNLOADQUEUE_NAME).toString();
                QString new_target = QFileDialog::getSaveFileName(this, tr("Choose filename"), itemTarget, tr("All files (*.*)"));

                if (!new_target.isEmpty() && new_target != itemTarget){
                    new_target = QDir::toNativeSeparators(new_target);
                    try {
                        QM->move(itemTarget.toStdString(), new_target.toStdString());
                    }
                    catch (const Exception &){}
                }
            }

            break;
        }
        case Menu::SetPriority:
        {
            for (const auto &i : items){
                QString itemTarget = i->data(COLUMN_DOWNLOADQUEUE_PATH).toString() + i->data(COLUMN_DOWNLOADQUEUE_NAME).toString();

                try {
                    QM->setPriority(itemTarget.toStdString(), static_cast<QueueItem::Priority>(arg.toInt()));
                }
                catch (const Exception&) {}
            }

            break;
        }
        case Menu::Browse:
        {
            rmap = arg.toMap();
            QString cid = getCID(rmap);

            if (d->sources.contains(target) && !cid.isEmpty()){
                UserPtr user = ClientManager::getInstance()->findUser(CID(cid.toStdString()));

                if (user){
                    try {
                        QM->addList(HintedUser(user, ""), QueueItem::FLAG_CLIENT_VIEW, "");
                    }
                    catch (const Exception&){}
                }
            }

            break;
        }
        case Menu::SendPM:
        {
            rmap = arg.toMap();
            auto it = rmap.constBegin();
            dcpp::CID cid(_tq(getCID(rmap)));
            QString nick = ((++it).key());
            QList<QObject*> hubs = HubManager::getInstance()->getHubs();

            for (const auto &obj : hubs){
                HubFrame *fr = qobject_cast<HubFrame*>(obj);

                if (!fr)
                    continue;

                if (fr->hasCID(cid, nick)){
                    fr->createPMWindow(cid);

                    break;
                }
            }

            break;
        }
        case Menu::RemoveSource:
        {
            rmap = arg.toMap();
            QString cid = getCID(rmap);

            if (d->sources.contains(target) && !cid.isEmpty()){
                UserPtr user = ClientManager::getInstance()->findUser(CID(cid.toStdString()));

                if (user){
                    try {
                        QM->removeSource(target.toStdString(), user, QueueItem::Source::FLAG_REMOVED);
                    }
                    catch (const Exception&){}
                }
            }

            break;
        }
        case Menu::RemoveUser:
        {
            rmap = arg.toMap();
            QString cid = getCID(rmap);

            if (d->sources.contains(target) && !cid.isEmpty()){
                UserPtr user = ClientManager::getInstance()->findUser(CID(cid.toStdString()));

                if (user){
                    try {
                        QM->removeSource(user, QueueItem::Source::FLAG_REMOVED);
                    }
                    catch (const Exception&){}
                }
            }

            break;
        }
        case Menu::Remove:
        {
            for (const auto &i : items){
                QString itemTarget = i->data(COLUMN_DOWNLOADQUEUE_PATH).toString() + i->data(COLUMN_DOWNLOADQUEUE_NAME).toString();

                try {
                    QM->remove(itemTarget.toStdString());
                }
                catch (const Exception &){}
            }

            break;
        }
        default:
            break;
    }
}
