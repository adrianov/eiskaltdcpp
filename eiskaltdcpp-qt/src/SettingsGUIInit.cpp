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
#include "MainWindow.h"
#include "Notification.h"
#include "EmoticonFactory.h"
#include "CustomFontModel.h"

#include "dcpp/SettingsManager.h"

#include <QListWidgetItem>
#include <QPixmap>
#include <QColor>
#include <QColorDialog>
#include <QStyleFactory>
#include <QFontDialog>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QSystemTrayIcon>
#include <QHeaderView>
#include <QMap>
#include <QTextEdit>

using namespace dcpp;

void SettingsGUI::init(){
    initBasic();

    {//Chat tab
        checkBox_CHATJOINS->setChecked(WBGET(WB_CHAT_SHOW_JOINS));
        checkBox_JOINSFAV->setChecked(WBGET(WB_CHAT_SHOW_JOINS_FAV));
        checkBox_CHATHIDDEN->setChecked(WBGET(WB_SHOW_HIDDEN_USERS));
        checkBox_IGNOREPMHUB->setChecked(BOOLSETTING(IGNORE_HUB_PMS));
        checkBox_IGNOREPMBOT->setChecked(BOOLSETTING(IGNORE_BOT_PMS));
        checkBox_REDIRECTPMBOT->setChecked(WBGET(WB_CHAT_REDIRECT_BOT_PMS));
        checkBox_REDIRECT_UNREAD->setChecked(WBGET("hubframe/redirect-pm-to-main-chat", false));
        checkBox_KEEPFOCUS->setChecked(WBGET(WB_CHAT_KEEPFOCUS));
        checkBox_UNREADEN_DRAW_LINE->setChecked(WBGET("hubframe/unreaden-draw-line", true));
        checkBox_USE_CTRL_ENTER->setChecked(WBGET(WB_USE_CTRL_ENTER));
        checkBox_ROTATING->setChecked(WBGET(WB_CHAT_ROTATING_MSGS));
        checkBox_EMOT->setChecked(WBGET(WB_APP_ENABLE_EMOTICON));
        checkBox_EMOTFORCE->setChecked(WBGET(WB_APP_FORCE_EMOTICONS));
        checkBox_SMILEPANEL->setChecked(WBGET(WB_CHAT_USE_SMILE_PANEL));
        checkBox_HIDESMILEPANEL->setChecked(WBGET(WB_CHAT_HIDE_SMILE_PANEL));
    }
    {//Chat (extended) tab
        comboBox_DBL_CLICK->setCurrentIndex(WIGET(WI_CHAT_DBLCLICK_ACT));
        comboBox_MDL_CLICK->setCurrentIndex(WIGET(WI_CHAT_MDLCLICK_ACT));
        comboBox_DEF_MAGNET_ACTION->setCurrentIndex(WIGET(WI_DEF_MAGNET_ACTION));
        comboBox_APP_UNIT_BASE->setCurrentIndex(SETTING(APP_UNIT_BASE));
        checkBox_HIGHLIGHTFAVS->setChecked(WBGET(WB_CHAT_HIGHLIGHT_FAVS));
        checkBox_CHAT_SHOW_IP->setChecked(BOOLSETTING(USE_IP));
        checkBox_CHAT_SHOW_CC->setChecked(BOOLSETTING(GET_USER_COUNTRY));
        checkBox_BB_CODE->setChecked(WBGET("hubframe/use-bb-code", true));
        lineEdit_TIMESTAMP->setText(WSGET(WS_CHAT_TIMESTAMP));

        spinBox_OUT_IN_HIST->setValue(WIGET(WI_OUT_IN_HIST));
        spinBox_PARAGRAPHS->setValue(WIGET(WI_CHAT_MAXPARAGRAPHS));

        comboBox_CHAT_SEPARATOR->setCurrentIndex(comboBox_CHAT_SEPARATOR->findText(WSGET(WS_CHAT_SEPARATOR)));
    }

    initColors();
}
