/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "statusbar/HashProgressBar.h"
#include "AppTheme.h"
#include "HashProgress.h"
#include "WulforUtil.h"

#include <QAction>
#include <QProgressBar>
#include <QStatusBar>

#include "dcpp/SettingsManager.h"
#include "dcpp/Util.h"

using namespace dcpp;

void HashProgressBar::build(QWidget *host, QObject *filter)
{
    bar = new QProgressBar(host);
    bar->setMaximum(100);
    bar->setMinimum(0);
    bar->setAlignment(Qt::AlignHCenter);
    bar->setFixedHeight(18);
    bar->setToolTip(QObject::tr("Hashing progress"));
    bar->hide();
    bar->installEventFilter(filter);
}

void HashProgressBar::mount(QStatusBar *barHost)
{
    if (bar)
        barHost->addWidget(bar);
}

void HashProgressBar::update(QAction *refreshAction, HashProgress *dialog)
{
    if (!bar)
        return;

    AppTheme::applyProgressBar(bar);

    switch (HashProgress::getHashStatus()) {
    case HashProgress::IDLE:
        WulforUtil::bindActionIcon(refreshAction, WulforUtil::eiREFRLIST);
        refreshAction->setText(QObject::tr("Refresh share"));
        if (dialog)
            dialog->resetProgress();
        bar->hide();
        break;
    case HashProgress::LISTUPDATE:
        WulforUtil::bindActionIcon(refreshAction, WulforUtil::eiHASHING);
        refreshAction->setText(QObject::tr("Hash progress"));
        bar->setValue(100);
        bar->setFormat(QObject::tr("List update"));
        bar->show();
        break;
    case HashProgress::DELAYED:
        WulforUtil::bindActionIcon(refreshAction, WulforUtil::eiHASHING);
        refreshAction->setText(QObject::tr("Hash progress"));
        {
            const int delay = SETTING(HASHING_START_DELAY);
            if (delay > 0) {
                const int left = delay - Util::getUpTime();
                bar->setValue(100 * left / delay);
                bar->setFormat(QObject::tr("Delayed"));
                bar->show();
            } else {
                bar->hide();
            }
        }
        break;
    case HashProgress::PAUSED:
        WulforUtil::bindActionIcon(refreshAction, WulforUtil::eiHASHING);
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
        WulforUtil::bindActionIcon(refreshAction, WulforUtil::eiHASHING);
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
