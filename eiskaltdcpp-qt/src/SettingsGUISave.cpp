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
#include "AppTheme.h"
#include "Notification.h"
#include "EmoticonFactory.h"

#include "dcpp/SettingsManager.h"

#include <QHeaderView>
#include <QTextEdit>

using namespace dcpp;

void SettingsGUI::ok(){
    SettingsManager *SM = SettingsManager::getInstance();
    {//Basic tab
        if (custom_style && comboBox_THEMES->currentIndex() > 0)
            WSSET(WS_APP_THEME, comboBox_THEMES->currentText());
        else if (!comboBox_THEMES->currentIndex()) {
            WSSET(WS_APP_THEME, "");
            AppTheme::applyPreferredStyle();
            AppTheme::apply();
        }

        WSSET(WS_TRANSLATION_FILE, lineEdit_LANGFILE->text());

        WBSET(WB_MAINWINDOW_REMEMBER, radioButton_REMEMBER->isChecked());
        WBSET(WB_MAINWINDOW_HIDE, radioButton_HIDE->isChecked());

        if (WBGET(WB_TRAY_ENABLED) != groupBox_TRAY->isChecked()){
            WBSET(WB_TRAY_ENABLED, groupBox_TRAY->isChecked());

            Notification::getInstance()->enableTray(WBGET(WB_TRAY_ENABLED));
        }

        if (WSGET(WS_APP_EMOTICON_THEME) != comboBox_EMOT->currentText()){
            WSSET(WS_APP_EMOTICON_THEME, comboBox_EMOT->currentText());

            if (EmoticonFactory::getInstance())
                EmoticonFactory::getInstance()->load();
        }

        if (comboBox_TABBAR->currentIndex() == 2){
            WBSET(WB_MAINWINDOW_USE_SIDEBAR, true);
            WBSET(WB_MAINWINDOW_USE_M_TABBAR, false);
        }
        else if (comboBox_TABBAR->currentIndex() == 1){
            WBSET(WB_MAINWINDOW_USE_SIDEBAR, false);
            WBSET(WB_MAINWINDOW_USE_M_TABBAR, true);
        }
        else{
            WBSET(WB_MAINWINDOW_USE_SIDEBAR, false);
            WBSET(WB_MAINWINDOW_USE_M_TABBAR, false);
        }

        WBSET("app/use-icon-theme", checkBox_ICONTHEME->isChecked());
        WBSET("mainwindow/dont-show-icons-in-menus", checkBox_HIDE_ICONS_IN_MENU->isChecked());
    }
    {//Chat tab
        WBSET(WB_SHOW_HIDDEN_USERS, checkBox_CHATHIDDEN->isChecked());
        WBSET(WB_CHAT_SHOW_JOINS, checkBox_CHATJOINS->isChecked());
        WBSET(WB_CHAT_SHOW_JOINS_FAV, checkBox_JOINSFAV->isChecked());
        WBSET(WB_CHAT_REDIRECT_BOT_PMS, checkBox_REDIRECTPMBOT->isChecked());
        WBSET("hubframe/redirect-pm-to-main-chat", checkBox_REDIRECT_UNREAD->isChecked());
        WBSET(WB_CHAT_KEEPFOCUS, checkBox_KEEPFOCUS->isChecked());
        WBSET("hubframe/unreaden-draw-line", checkBox_UNREADEN_DRAW_LINE->isChecked());
        WBSET(WB_CHAT_ROTATING_MSGS, checkBox_ROTATING->isChecked());
        WBSET(WB_USE_CTRL_ENTER, checkBox_USE_CTRL_ENTER->isChecked());
        WBSET(WB_APP_ENABLE_EMOTICON, checkBox_EMOT->isChecked());
        WBSET(WB_APP_FORCE_EMOTICONS, checkBox_EMOTFORCE->isChecked());
        WBSET(WB_CHAT_USE_SMILE_PANEL, checkBox_SMILEPANEL->isChecked());
        WBSET(WB_CHAT_HIDE_SMILE_PANEL, checkBox_HIDESMILEPANEL->isChecked());
    }
    {//Chat (extended) tab
        WISET(WI_CHAT_DBLCLICK_ACT, comboBox_DBL_CLICK->currentIndex());
        WISET(WI_CHAT_MDLCLICK_ACT, comboBox_MDL_CLICK->currentIndex());
        WISET(WI_DEF_MAGNET_ACTION, comboBox_DEF_MAGNET_ACTION->currentIndex());
        SM->set(SettingsManager::APP_UNIT_BASE, comboBox_APP_UNIT_BASE->currentIndex());
        WBSET(WB_CHAT_HIGHLIGHT_FAVS, checkBox_HIGHLIGHTFAVS->isChecked());
        SM->set(SettingsManager::USE_IP, checkBox_CHAT_SHOW_IP->isChecked());
        WBSET("hubframe/use-bb-code", checkBox_BB_CODE->isChecked());

        WSSET(WS_CHAT_TIMESTAMP, lineEdit_TIMESTAMP->text());

        WISET(WI_OUT_IN_HIST, spinBox_OUT_IN_HIST->value());
        WISET(WI_CHAT_MAXPARAGRAPHS, spinBox_PARAGRAPHS->value());

        SM->set(SettingsManager::IGNORE_BOT_PMS, checkBox_IGNOREPMBOT->isChecked());
        SM->set(SettingsManager::IGNORE_HUB_PMS, checkBox_IGNOREPMHUB->isChecked());
        SM->set(SettingsManager::GET_USER_COUNTRY, checkBox_CHAT_SHOW_CC->isChecked());
        WSSET(WS_CHAT_SEPARATOR, comboBox_CHAT_SEPARATOR->currentText());
    }
    {//Color tab
        int i = 0;

        WSSET(WS_CHAT_LOCAL_COLOR,      QColor(listWidget_CHATCOLOR->item(i++)->icon().pixmap(10, 10).toImage().pixel(0, 0)).name());
        WSSET(WS_CHAT_OP_COLOR,         QColor(listWidget_CHATCOLOR->item(i++)->icon().pixmap(10, 10).toImage().pixel(0, 0)).name());
        WSSET(WS_CHAT_BOT_COLOR,        QColor(listWidget_CHATCOLOR->item(i++)->icon().pixmap(10, 10).toImage().pixel(0, 0)).name());
        WSSET(WS_CHAT_PRIV_LOCAL_COLOR, QColor(listWidget_CHATCOLOR->item(i++)->icon().pixmap(10, 10).toImage().pixel(0, 0)).name());
        WSSET(WS_CHAT_PRIV_USER_COLOR,  QColor(listWidget_CHATCOLOR->item(i++)->icon().pixmap(10, 10).toImage().pixel(0, 0)).name());
        WSSET(WS_CHAT_SAY_NICK,         QColor(listWidget_CHATCOLOR->item(i++)->icon().pixmap(10, 10).toImage().pixel(0, 0)).name());
        WSSET(WS_CHAT_STAT_COLOR,       QColor(listWidget_CHATCOLOR->item(i++)->icon().pixmap(10, 10).toImage().pixel(0, 0)).name());
        WSSET(WS_CHAT_USER_COLOR,       QColor(listWidget_CHATCOLOR->item(i++)->icon().pixmap(10, 10).toImage().pixel(0, 0)).name());
        WSSET(WS_CHAT_FAVUSER_COLOR,    QColor(listWidget_CHATCOLOR->item(i++)->icon().pixmap(10, 10).toImage().pixel(0, 0)).name());
        WSSET(WS_CHAT_TIME_COLOR,       QColor(listWidget_CHATCOLOR->item(i++)->icon().pixmap(10, 10).toImage().pixel(0, 0)).name());
        WSSET(WS_CHAT_MSG_COLOR,        QColor(listWidget_CHATCOLOR->item(i++)->icon().pixmap(10, 10).toImage().pixel(0, 0)).name());

        WSSET(WS_CHAT_FIND_COLOR,       h_color.name());
        WISET(WI_CHAT_FIND_COLOR_ALPHA, horizontalSlider_H_COLOR->value());

        WSSET(WS_APP_SHARED_FILES_COLOR, shared_files_color.name());
        WISET(WI_APP_SHARED_FILES_ALPHA, horizontalSlider_SHAREDFILES->value());

        WSSET(WS_APP_QUEUED_FILES_COLOR, queued_files_color.name());
        WISET(WI_APP_QUEUED_FILES_ALPHA, horizontalSlider_QUEUEDFILES->value());

        WBSET("hubframe/change-chat-background-color", checkBox_CHAT_BACKGROUND_COLOR->isChecked());
        if (chat_background_color.isValid())
            WSSET("hubframe/chat-background-color", chat_background_color.name());
        if (!checkBox_CHAT_BACKGROUND_COLOR->isChecked())
            WSSET("hubframe/chat-background-color", QTextEdit().palette().color(QPalette::Active, QPalette::Base).name());
        if (downloads_clr.isValid())
            WVSET("transferview/download-bar-color", downloads_clr);
        if (uploads_clr.isValid())
            WVSET("transferview/upload-bar-color", uploads_clr);
    }

    WSSET(WS_SETTINGS_GUI_FONTS_STATE, tableView->horizontalHeader()->saveState().toBase64());

    emit saveFonts();
}

