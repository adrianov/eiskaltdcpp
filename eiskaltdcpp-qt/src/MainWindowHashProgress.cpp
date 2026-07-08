/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "MainWindowHashProgress.h"
#include "HashProgress.h"
#include "WulforUtil.h"

#include <QAction>
#include <QProgressBar>

#include "dcpp/Util.h"

using namespace dcpp;

namespace {

void bindActionIcon(QAction *act, WulforUtil::Icons icon)
{
    act->setProperty("wulforIcon", icon);
    act->setIcon(WulforUtil::getInstance()->getIcon(icon));
}

}

void MainWindowHashProgress::update(QProgressBar *bar, QAction *refreshAction, HashProgress *dialog)
{
    switch (HashProgress::getHashStatus()) {
    case HashProgress::IDLE:
        bindActionIcon(refreshAction, WulforUtil::eiREFRLIST);
        refreshAction->setText(QObject::tr("Refresh share"));
        if (dialog)
            dialog->resetProgress();
        bar->hide();
        break;
    case HashProgress::LISTUPDATE:
        bindActionIcon(refreshAction, WulforUtil::eiHASHING);
        refreshAction->setText(QObject::tr("Hash progress"));
        bar->setValue(100);
        bar->setFormat(QObject::tr("List update"));
        bar->show();
        break;
    case HashProgress::DELAYED:
        bindActionIcon(refreshAction, WulforUtil::eiHASHING);
        refreshAction->setText(QObject::tr("Hash progress"));
        if (SETTING(HASHING_START_DELAY) >= 0) {
            const int left = SETTING(HASHING_START_DELAY) - Util::getUpTime();
            bar->setValue(100 * left / SETTING(HASHING_START_DELAY));
            bar->setFormat(QObject::tr("Delayed"));
            bar->show();
        }
        else
            bar->hide();
        break;
    case HashProgress::PAUSED:
        bindActionIcon(refreshAction, WulforUtil::eiHASHING);
        refreshAction->setText(QObject::tr("Hash progress"));
        if (SETTING(HASHING_START_DELAY) >= 0) {
            bar->setValue(100);
            bar->setFormat(QObject::tr("Paused"));
            bar->show();
        }
        else
            bar->hide();
        break;
    case HashProgress::RUNNING:
        bindActionIcon(refreshAction, WulforUtil::eiHASHING);
        refreshAction->setText(QObject::tr("Hash progress"));
        if (dialog) {
            bar->setFormat(QObject::tr("%p%"));
            bar->setValue(static_cast<int>(dialog->getProgress() * 100));
            bar->show();
        }
        break;
    default:
        break;
    }
}
