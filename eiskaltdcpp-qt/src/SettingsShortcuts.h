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

#include "ui_UISettingsShortcuts.h"
#include "ShortcutsModel.h"

class SettingsShortcuts : public QWidget, private Ui::UISettingsShortcuts
{
    Q_OBJECT
public:
    explicit SettingsShortcuts(QWidget *parent = nullptr);
    ~SettingsShortcuts() override;

public Q_SLOTS:
    void ok();

private Q_SLOTS:
    void slotIndexClicked(const QModelIndex&);

private:
    ShortcutsModel *model;
};
