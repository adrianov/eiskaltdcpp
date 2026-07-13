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
#include "WulforUtil.h"
#include "AppTheme.h"
#include "CustomFontModel.h"

#include <QListWidgetItem>
#include <QPixmap>
#include <QColor>
#include <QHeaderView>

void SettingsGUI::initColors()
{
    {//Color tab
        QColor c;
        QPixmap p(10, 10);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_LOCAL_COLOR));
        p.fill(c);
        new QListWidgetItem(p, tr("Local user"), listWidget_CHATCOLOR);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_OP_COLOR));
        p.fill(c);
        new QListWidgetItem(p, tr("Operator"), listWidget_CHATCOLOR);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_BOT_COLOR));
        p.fill(c);
        new QListWidgetItem(p, tr("Bot"), listWidget_CHATCOLOR);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_PRIV_LOCAL_COLOR));
        p.fill(c);
        new QListWidgetItem(p, tr("Private: local user"), listWidget_CHATCOLOR);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_PRIV_USER_COLOR));
        p.fill(c);
        new QListWidgetItem(p, tr("Private: user"), listWidget_CHATCOLOR);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_SAY_NICK));
        p.fill(c);
        new QListWidgetItem(p, tr("Chat: Say nick"), listWidget_CHATCOLOR);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_STAT_COLOR));
        p.fill(c);
        new QListWidgetItem(p, tr("Status"), listWidget_CHATCOLOR);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_USER_COLOR));
        p.fill(c);
        new QListWidgetItem(p, tr("User"), listWidget_CHATCOLOR);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_FAVUSER_COLOR));
        p.fill(c);
        new QListWidgetItem(p, tr("Favorite User"), listWidget_CHATCOLOR);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_TIME_COLOR));
        p.fill(c);
        new QListWidgetItem(p, tr("Time stamp"), listWidget_CHATCOLOR);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_MSG_COLOR));
        p.fill(c);
        new QListWidgetItem(p, tr("Message"), listWidget_CHATCOLOR);

        c.setNamedColor(AppTheme::chatColor(WS_CHAT_FIND_COLOR));
        h_color = c;

        c.setAlpha(WIGET(WI_CHAT_FIND_COLOR_ALPHA));
        p.fill(c);
        toolButton_H_COLOR->setIcon(p);

        c.setNamedColor(WSGET(WS_APP_SHARED_FILES_COLOR));
        shared_files_color = c;
        c.setAlpha(WIGET(WI_APP_SHARED_FILES_ALPHA));
        p.fill(c);
        toolButton_SHAREDFILES->setIcon(p);

        c.setNamedColor(AppTheme::chatColor(WS_APP_QUEUED_FILES_COLOR));
        queued_files_color = c;
        c.setAlpha(WIGET(WI_APP_QUEUED_FILES_ALPHA, 56));
        p.fill(c);
        toolButton_QUEUEDFILES->setIcon(p);

        downloads_clr = qvariant_cast<QColor>(WVGET("transferview/download-bar-color", QColor()));
        uploads_clr = qvariant_cast<QColor>(WVGET("transferview/upload-bar-color", QColor()));

        if (downloads_clr.isValid()){
            c = downloads_clr;
            p.fill(c);

            toolButton_DOWNLOADSCLR->setIcon(p);
        }
        if (uploads_clr.isValid()){
            c = uploads_clr;
            p.fill(c);

            toolButton_UPLOADSCLR->setIcon(p);
        }

        checkBox_CHAT_BACKGROUND_COLOR->setChecked(WBGET("hubframe/change-chat-background-color", false));
        toolButton_CHAT_BACKGROUND_COLOR->setEnabled(WBGET("hubframe/change-chat-background-color", false));
        if (!WSGET("hubframe/chat-background-color", "").isEmpty()){
            c.setNamedColor(WSGET("hubframe/chat-background-color"));
            chat_background_color = c;
            c.setAlpha(255);
            p.fill(c);
            toolButton_CHAT_BACKGROUND_COLOR->setIcon(p);
        }

        horizontalSlider_H_COLOR->setValue(WIGET(WI_CHAT_FIND_COLOR_ALPHA));
        horizontalSlider_SHAREDFILES->setValue(WIGET(WI_APP_SHARED_FILES_ALPHA));
        horizontalSlider_QUEUEDFILES->setValue(WIGET(WI_APP_QUEUED_FILES_ALPHA, 56));
    }
    {// Fonts tab
        CustomFontModel *model = new CustomFontModel(this);
        tableView->setModel(model);

        WulforUtil::restoreTreeHeader(tableView->horizontalHeader(), QByteArray::fromBase64(WSGET(WS_SETTINGS_GUI_FONTS_STATE).toUtf8()));

        connect(tableView, SIGNAL(doubleClicked(QModelIndex)), model, SLOT(itemDoubleClicked(QModelIndex)));
        connect(this, SIGNAL(saveFonts()), model, SLOT(ok()));
    }

    connect(pushButton_TEST, SIGNAL(clicked()), this, SLOT(slotTestAppTheme()));
    connect(comboBox_THEMES, SIGNAL(activated(int)), this, SLOT(slotThemeChanged()));
    connect(listWidget_CHATCOLOR, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(slotChatColorItemClicked(QListWidgetItem*)));
    connect(toolButton_LANGBROWSE, SIGNAL(clicked()), this, SLOT(slotBrowseLng()));
    connect(comboBox_LANGS, SIGNAL(activated(int)), this, SLOT(slotLngIndexChanged(int)));
    connect(comboBox_USERS, SIGNAL(activated(int)), this, SLOT(slotUsersChanged()));
    connect(comboBox_ICONS, SIGNAL(activated(int)), this, SLOT(slotIconsChanged()));
    connect(toolButton_H_COLOR, SIGNAL(clicked()), this, SLOT(slotGetColor()));
    connect(toolButton_SHAREDFILES, SIGNAL(clicked()), this, SLOT(slotGetColor()));
    connect(toolButton_QUEUEDFILES, SIGNAL(clicked()), this, SLOT(slotGetColor()));
    connect(toolButton_CHAT_BACKGROUND_COLOR, SIGNAL(clicked()), this, SLOT(slotGetColor()));
    connect(toolButton_DOWNLOADSCLR, SIGNAL(clicked()), this, SLOT(slotGetColor()));
    connect(toolButton_UPLOADSCLR, SIGNAL(clicked()), this, SLOT(slotGetColor()));
    connect(pushButton_RESET, SIGNAL(clicked()), this, SLOT(slotResetTransferColors()));
    connect(horizontalSlider_H_COLOR, SIGNAL(valueChanged(int)), this, SLOT(slotSetTransparency(int)));
    connect(horizontalSlider_SHAREDFILES, SIGNAL(valueChanged(int)), this, SLOT(slotSetTransparency(int)));
    connect(horizontalSlider_QUEUEDFILES, SIGNAL(valueChanged(int)), this, SLOT(slotSetTransparency(int)));
}
