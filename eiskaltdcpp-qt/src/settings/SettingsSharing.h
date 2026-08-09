/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QWidget>
#include <QShowEvent>
#include <QHeaderView>

#include "ui_UISettingsSharing.h"

class ShareDirsPane;

class SettingsSharing :
        public QWidget,
        private Ui::UISettingsSharing
{
    Q_OBJECT
public:
    SettingsSharing(QWidget *parent = nullptr);
    ~SettingsSharing() override = default;

protected:
    void showEvent(QShowEvent *) override;

public Q_SLOTS:
    void ok();

private slots:
    void slotRecreateShare();
    void slotShareHidden(bool);
    void slotHeaderMenu();
    void slotAddException();
    void slotEditException();
    void slotDeleteException();
    void slotAddDirException();
    void slotSimpleShareModeChanged();
    void slotContextMenu(const QPoint &);

private:
    void init();

    ShareDirsPane *dirs = nullptr;
};
