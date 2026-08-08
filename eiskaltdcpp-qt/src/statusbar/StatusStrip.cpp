/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "statusbar/StatusStrip.h"
#include "statusbar/StatusBarLogLabel.h"
#include "WulforSettings.h"

#include <QFontMetrics>
#include <QFrame>
#include <QLabel>
#include <QObject>
#include <QStatusBar>

void StatusStrip::build(QStatusBar *bar, QWidget *host, QObject *filter)
{
    counts = new QLabel(bar);
    counts->setFrameShadow(QFrame::Sunken);
    counts->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    counts->setToolTip(QObject::tr("Counts"));
    counts->setContentsMargins(0, 0, 0, 0);

    speed = new QLabel(bar);
    speed->setFrameShadow(QFrame::Sunken);
    speed->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    speed->setToolTip(QObject::tr("Download/Upload speed"));
    speed->setContentsMargins(0, 0, 0, 0);

    volume = new QLabel(bar);
    volume->setFrameShadow(QFrame::Sunken);
    volume->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    volume->setToolTip(QObject::tr("Downloaded/Uploaded"));
    volume->setContentsMargins(0, 0, 0, 0);

    log = new StatusBarLogLabel(bar);
    log->setHeightRef(counts);

    freeSpace.build(host, filter);
    hashing.build(host, filter);

    hashing.mount(bar);
    bar->addWidget(log, 1);
    bar->addPermanentWidget(volume);
    bar->addPermanentWidget(speed);
    bar->addPermanentWidget(counts);
    freeSpace.mount(bar);
}

void StatusStrip::updateStats(const QMap<QString, QString> &map)
{
    if (!counts)
        return;

    const QString statsText = map.value(QStringLiteral("STATS"));
    const QString speedText = QObject::tr("%1/s / %2/s")
            .arg(map.value(QStringLiteral("DSPEED")), map.value(QStringLiteral("USPEED")));
    const QString volumeText = QObject::tr("%1 / %2")
            .arg(map.value(QStringLiteral("DOWN")), map.value(QStringLiteral("UP")));

    counts->setText(statsText);
    speed->setText(speedText);
    volume->setText(volumeText);

    const QFontMetrics metrics(counts->font());
    const QMargins margins = speed->contentsMargins();
    const int pad = margins.left() + margins.right();

    const int speedNeed = metrics.horizontalAdvance(speedText) + pad;
    if (speedNeed > speed->width())
        speed->setFixedWidth(speedNeed);

    const int volumeNeed = metrics.horizontalAdvance(volumeText) + pad;
    if (volumeNeed > volume->width())
        volume->setFixedWidth(volumeNeed);

    freeSpace.refresh();
}

void StatusStrip::setLogMessage(const QString &msg)
{
    if (log)
        log->setLogMessage(msg, WIGET(WI_STATUSBAR_HISTORY_SZ));
}

void StatusStrip::updateHashing(QAction *refreshAction, HashProgress *dialog)
{
    hashing.update(refreshAction, dialog);
}

void StatusStrip::toggleFreeSpace(QAction *action)
{
    freeSpace.toggle(action);
}

void StatusStrip::syncFreeSpaceAction(QAction *action)
{
    freeSpace.syncAction(action);
}
