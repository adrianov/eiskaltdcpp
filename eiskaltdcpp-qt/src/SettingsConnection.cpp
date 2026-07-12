/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "SettingsConnection.h"

#include "dcpp/stdinc.h"

#include <QMessageBox>

#include "QtCompat.h"

using namespace dcpp;

SettingsConnection::SettingsConnection(QWidget *parent):
        QWidget(parent),
        dirty(false)
{
    setupUi(this);
    init();
}

SettingsConnection::~SettingsConnection() {
    shutdownPortDots();
}

bool SettingsConnection::eventFilter(QObject *obj, QEvent *e) {
    if((e->type() == QEvent::KeyRelease) || (e->type() == QEvent::MouseButtonRelease))
        dirty = true;
    return QWidget::eventFilter(obj, e);
}

void SettingsConnection::slotToggleIncomming() {
    bool b = !radioButton_PASSIVE->isChecked();
    frame->setEnabled(b);
    pushButton_TEST_PORTS->setEnabled(b);
}

void SettingsConnection::slotToggleOutgoing() {
    frame_2->setEnabled(!radioButton_DC->isChecked());
}

bool SettingsConnection::validateIp(QString &ip) {
    if(ip.isEmpty() || ip.isNull())
        return false;

    QStringList l = ip.split(".", WULFOR_SKIP_EMPTY);
    if(l.size() != 4)
        return false;

    QIntValidator v(0, 255, this);
    bool valid = true;
    int pos = 0;
    for(QString s : l)
        valid = valid && (v.validate(s, pos) == QValidator::Acceptable);
    return valid;
}

void SettingsConnection::showMsg(QString msg, QWidget *focusTo) {
    QMessageBox::warning(this, tr("Warning"), msg);
    if(focusTo)
        focusTo->setFocus();
}
