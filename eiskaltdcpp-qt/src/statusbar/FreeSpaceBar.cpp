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

#include "statusbar/FreeSpaceBar.h"
#include "AppTheme.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include <QAction>
#include <QFontMetrics>
#include <QProgressBar>
#include <QStatusBar>

#ifdef FREE_SPACE_BAR_C
#include "dcpp/SettingsManager.h"
#include "extra/freespace.h"

using namespace dcpp;
#endif

namespace {
constexpr unsigned long long kGiB = 1ULL << 30;
constexpr unsigned long long kWarnFree = 10 * kGiB;
constexpr unsigned long long kCritFree = 1 * kGiB;

QString showText() { return QObject::tr("Show free space"); }
QString hideText() { return QObject::tr("Hide free space"); }

QColor chunkFor(unsigned long long freeBytes)
{
    if (freeBytes < kCritFree)
        return AppTheme::errorColor();
    if (freeBytes < kWarnFree)
        return AppTheme::warningColor();
    return AppTheme::linkColor();
}
} // namespace

void FreeSpaceBar::build(QWidget *host, QObject *filter)
{
#ifdef FREE_SPACE_BAR_C
    bar = new QProgressBar(host);
    bar->setMaximum(100);
    bar->setMinimum(0);
    bar->setAlignment(Qt::AlignHCenter);
    bar->setMinimumWidth(100);
    bar->setMaximumWidth(250);
    bar->setFixedHeight(18);
    bar->setToolTip(QObject::tr("Free disk space"));
    bar->installEventFilter(filter);

    if (!WBGET(WB_SHOW_FREE_SPACE))
        bar->hide();
#else
    Q_UNUSED(host)
    Q_UNUSED(filter)
    WBSET(WB_SHOW_FREE_SPACE, false);
#endif
}

void FreeSpaceBar::mount(QStatusBar *barHost)
{
#ifdef FREE_SPACE_BAR_C
    if (bar)
        barHost->addPermanentWidget(bar);
#else
    Q_UNUSED(barHost)
#endif
}

void FreeSpaceBar::refresh()
{
#ifdef FREE_SPACE_BAR_C
    if (!bar || !WBGET(WB_SHOW_FREE_SPACE))
        return;

    unsigned long long available = 0;
    unsigned long long total = 0;
    const std::string path = SETTING(DOWNLOAD_DIRECTORY);
    if (!path.empty() && !FreeSpace::FreeDiscSpace(path, &available, &total)) {
        available = 0;
        total = 0;
    }

    const QString text = QObject::tr("Free %1").arg(WulforUtil::formatBytes(available));
    const QString tip = QObject::tr("%1 free of %2")
            .arg(WulforUtil::formatBytes(available), WulforUtil::formatBytes(total));

    const int usedPct = (total > 0)
            ? static_cast<int>(100.0 * static_cast<double>(total - available) / static_cast<double>(total))
            : 0;

    bar->setFormat(text);
    bar->setValue(usedPct);
    bar->setToolTip(tip);
    AppTheme::applyProgressBar(bar, chunkFor(available));

    const QFontMetrics metrics(bar->font());
    const int need = metrics.horizontalAdvance(text) + 40;
    if (need > bar->width())
        bar->setFixedWidth(need);
#endif
}

void FreeSpaceBar::toggle(QAction *action)
{
#ifdef FREE_SPACE_BAR_C
    if (!bar)
        return;

    const bool show = !WBGET(WB_SHOW_FREE_SPACE);
    WBSET(WB_SHOW_FREE_SPACE, show);
    bar->setVisible(show);
    if (action)
        action->setText(show ? hideText() : showText());
    if (show)
        refresh();
#else
    Q_UNUSED(action)
#endif
}

void FreeSpaceBar::syncAction(QAction *action)
{
    if (!action)
        return;
#ifdef FREE_SPACE_BAR_C
    action->setText(WBGET(WB_SHOW_FREE_SPACE) ? hideText() : showText());
#else
    action->setVisible(false);
#endif
}
