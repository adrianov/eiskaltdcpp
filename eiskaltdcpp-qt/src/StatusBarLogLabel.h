/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QLabel>
#include <QStringList>

class StatusBarLogLabel : public QLabel
{
    Q_OBJECT

public:
    explicit StatusBarLogLabel(QWidget *parent = nullptr);
    void setLogMessage(QString msg, int historySize);
    void setHeightRef(QLabel *ref) { heightRef = ref; }

protected:
    bool event(QEvent *e) override;

private:
    void refreshDisplay();
    void syncHeight();

    QStringList history;
    QString toolTipText;
    QString pendingText;
    bool hovered = false;
    QLabel *heightRef = nullptr;
};
