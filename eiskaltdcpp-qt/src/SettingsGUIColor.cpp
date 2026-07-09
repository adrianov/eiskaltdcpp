/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SettingsGUI.h"
#include "WulforSettings.h"

#include <QColorDialog>
#include <QListWidgetItem>
#include <QPixmap>

void SettingsGUI::slotChatColorItemClicked(QListWidgetItem *item)
{
    QPixmap p(10, 10);
    QColor color(item->icon().pixmap(10, 10).toImage().pixel(0, 0));
    color = QColorDialog::getColor(color);

    if (color.isValid()) {
        p.fill(color);
        item->setIcon(p);
    }
}

void SettingsGUI::slotGetColor()
{
    QPixmap p(10, 10);

    if (sender() == toolButton_H_COLOR) {
        QColor color = QColorDialog::getColor(h_color);
        if (color.isValid()) {
            h_color = color;
            color.setAlpha(horizontalSlider_H_COLOR->value());
            p.fill(color);
            toolButton_H_COLOR->setIcon(p);
        }
    } else if (sender() == toolButton_SHAREDFILES) {
        QColor color = QColorDialog::getColor(shared_files_color);
        if (color.isValid()) {
            shared_files_color = color;
            color.setAlpha(horizontalSlider_SHAREDFILES->value());
            p.fill(color);
            toolButton_SHAREDFILES->setIcon(p);
        }
    } else if (sender() == toolButton_CHAT_BACKGROUND_COLOR) {
        QColor color = QColorDialog::getColor(chat_background_color);
        if (color.isValid()) {
            chat_background_color = color;
            color.setAlpha(255);
            p.fill(color);
            toolButton_CHAT_BACKGROUND_COLOR->setIcon(p);
        }
    } else if (sender() == toolButton_DOWNLOADSCLR) {
        QColor color = QColorDialog::getColor(chat_background_color);
        if (color.isValid()) {
            downloads_clr = color;
            color.setAlpha(255);
            p.fill(color);
            toolButton_DOWNLOADSCLR->setIcon(p);
        }
    } else if (sender() == toolButton_UPLOADSCLR) {
        QColor color = QColorDialog::getColor(chat_background_color);
        if (color.isValid()) {
            uploads_clr = color;
            color.setAlpha(255);
            p.fill(color);
            toolButton_UPLOADSCLR->setIcon(p);
        }
    }
}

void SettingsGUI::slotSetTransparency(int value)
{
    QPixmap p(10, 10);
    QColor color = (sender() == horizontalSlider_H_COLOR) ? h_color : shared_files_color;
    color.setAlpha(value);
    if (color.isValid())
        p.fill(color);
    if (sender() == horizontalSlider_H_COLOR)
        toolButton_H_COLOR->setIcon(p);
    else
        toolButton_SHAREDFILES->setIcon(p);
}

void SettingsGUI::slotResetTransferColors()
{
    WVSET("transferview/download-bar-color", QColor());
    WVSET("transferview/upload-bar-color", QColor());
    downloads_clr = QColor();
    uploads_clr = QColor();
    toolButton_DOWNLOADSCLR->setIcon(QIcon());
    toolButton_UPLOADSCLR->setIcon(QIcon());
}
