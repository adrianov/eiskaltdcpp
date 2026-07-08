/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "MainWindowHashProgress.h"
#include "AppTheme.h"
#include "HashProgress.h"
#include "WulforUtil.h"

#include <QAction>
#include <QApplication>
#include <QPalette>
#include <QProgressBar>

#include "dcpp/Util.h"

using namespace dcpp;

namespace {

QString statusProgressStyle()
{
    const QPalette palette = qApp->palette();
    return QString(
        "QProgressBar {"
            "border: 1px solid %1;"
            "border-radius: 4px;"
            "background-color: %2;"
            "color: %3;"
            "text-align: center;"
            "padding: 0px;"
        "}"
        "QProgressBar::chunk {"
            "background-color: %4;"
            "border-radius: 3px;"
        "}"
    ).arg(palette.color(QPalette::Mid).name(),
          palette.color(QPalette::Base).name(),
          palette.color(QPalette::Text).name(),
          AppTheme::linkColor().name());
}

void applyStatusProgressStyle(QProgressBar *bar)
{
    const QString style = statusProgressStyle();
    if (bar->styleSheet() != style)
        bar->setStyleSheet(style);
}

}

void MainWindowHashProgress::update(QProgressBar *bar, QAction *refreshAction, HashProgress *dialog)
{
    applyStatusProgressStyle(bar);

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
