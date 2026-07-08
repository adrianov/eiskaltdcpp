/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "StatusBarLogLabel.h"
#include "WulforUtil.h"

#include <QEvent>
#include <QFontMetrics>

StatusBarLogLabel::StatusBarLogLabel(QWidget *parent)
    : QLabel(parent)
{
    setFrameShadow(QFrame::Plain);
    setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setWordWrap(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setContentsMargins(0, 0, 0, 0);
}

void StatusBarLogLabel::setLogMessage(QString msg, int historySize)
{
    QString pure = msg;
    QFontMetrics m(font());

    if (m.horizontalAdvance(msg) > width())
        pure = m.elidedText(msg, Qt::ElideRight, width(), 0);

    WulforUtil::getInstance()->textToHtml(pure, true);
    WulforUtil::getInstance()->textToHtml(msg, true);

    pendingText = pure;
    history.push_back(msg);

    if (historySize > 0) {
        while (history.size() > historySize)
            history.removeFirst();
    }
    else
        history.clear();

    toolTipText = history.join("\n");
    setText(pendingText);
    syncHeight();
    if (!hovered)
        setToolTip(toolTipText);
}

void StatusBarLogLabel::syncHeight()
{
    if (!heightRef)
        return;

    const int h = heightRef->height();
    if (maximumHeight() != h)
        setMaximumHeight(h);
}

bool StatusBarLogLabel::event(QEvent *e)
{
    if (e->type() == QEvent::Enter)
        hovered = true;
    else if (e->type() == QEvent::Leave) {
        hovered = false;
        setToolTip(toolTipText);
    }

    return QLabel::event(e);
}
