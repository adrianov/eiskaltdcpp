/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforSettings.h"

#include <QApplication>
#include <QPalette>
#include <QColor>

void WulforSettings::fillOldConfigMaps() {
    strmap.insert(WS_CHAT_LOCAL_COLOR,      "#078010");
    strmap.insert(WS_CHAT_OP_COLOR,         "#000000");
    strmap.insert(WS_CHAT_BOT_COLOR,        "#838383");
    strmap.insert(WS_CHAT_FIND_COLOR,       "#FFFF00");
    strmap.insert(WS_CHAT_PRIV_LOCAL_COLOR, "#078010");
    strmap.insert(WS_CHAT_PRIV_USER_COLOR,  "#ac0000");
    strmap.insert(WS_CHAT_SAY_NICK,         "#2344e7");
    strmap.insert(WS_CHAT_CORE_COLOR,       "#ff0000");
    strmap.insert(WS_CHAT_STAT_COLOR,       "#ac0000");
    strmap.insert(WS_CHAT_USER_COLOR,       "#ac0000");
    strmap.insert(WS_CHAT_FAVUSER_COLOR,    "#00ff7f");
    strmap.insert(WS_CHAT_TIME_COLOR,       qApp->palette().text().color().name());
    strmap.insert(WS_CHAT_MSG_COLOR,        qApp->palette().text().color().name());
    strmap.insert(WS_CHAT_USERLIST_STATE,   "");
    strmap.insert(WS_CHAT_CMD_ALIASES,      "");
    strmap.insert(WS_CHAT_FONT,             "");
    strmap.insert(WS_CHAT_ULIST_FONT,       "");
    strmap.insert(WS_CHAT_PM_FONT,          "");
    strmap.insert(WS_CHAT_SEPARATOR,        ":");
    strmap.insert(WS_CHAT_TIMESTAMP,        "hh:mm:ss");
    strmap.insert(WS_QCONNECT_HISTORY,      "");
    strmap.insert(WS_DEFAULT_LOCALE,        QStringLiteral("WINDOWS-1251"));
    strmap.insert(WS_DOWNLOAD_DIR_HISTORY,  "");
    strmap.insert(WS_DQUEUE_STATE,          "");
    strmap.insert(WS_SEARCH_STATE,          "");
    strmap.insert(WS_SEARCH_HISTORY,        "");
    strmap.insert(WS_TRANSLATION_FILE,      "");
    strmap.insert(WS_TRANSFERS_STATE,       "");
    strmap.insert(WS_SHARE_LPANE_STATE,     "");
    strmap.insert(WS_SHARE_RPANE_STATE,     "");
    strmap.insert(WS_MAINWINDOW_STATE,      "");
    strmap.insert(WS_MAINWINDOW_TOOLBAR_ACTS,"");
    strmap.insert(WS_FTRANSFERS_FILES_STATE,"");
    strmap.insert(WS_FTRANSFERS_USERS_STATE,"");
    strmap.insert(WS_FAV_HUBS_STATE,        "");
    strmap.insert(WS_ADLS_STATE,            "");
    strmap.insert(WS_APP_THEME,             "");
    strmap.insert(WS_APP_ICONTHEME,         "default");
    strmap.insert(WS_APP_USERTHEME,         "default");
    strmap.insert(WS_APP_SHARED_FILES_COLOR,"#1f8f1f");
    strmap.insert(WS_NOTIFY_SOUNDS,         "");
    strmap.insert(WS_NOTIFY_SND_CMD,        "");
    strmap.insert(WS_FAVUSERS_STATE,        "");
    strmap.insert(WS_SHAREHEADER_STATE,     "");
    strmap.insert(WS_DOWNLOADTO_ALIASES,    "");
    strmap.insert(WS_DOWNLOADTO_PATHS,      "");
    strmap.insert(WS_APP_EMOTICON_THEME,    "default");
    strmap.insert(WS_APP_ASPELL_LANG,       "");
    strmap.insert(WS_APP_ENABLED_SCRIPTS,   "");
    strmap.insert(WS_PUBLICHUBS_STATE,      "");
    strmap.insert(WS_SETTINGS_GUI_FONTS_STATE, "");

    intmap.insert(WB_APP_AUTOAWAY_BY_TIMER, static_cast<int>(false));
    intmap.insert(WB_CHAT_SHOW_TIMESTAMP,   static_cast<int>(true));
    intmap.insert(WB_SHOW_FREE_SPACE,       static_cast<int>(true));
    intmap.insert(WB_LAST_STATUS,           static_cast<int>(true));
    intmap.insert(WB_USERS_STATISTICS,      static_cast<int>(true));
    intmap.insert(WB_CHAT_SHOW_JOINS,       static_cast<int>(false));
    intmap.insert(WB_CHAT_REDIRECT_BOT_PMS, static_cast<int>(true));
    intmap.insert(WB_CHAT_KEEPFOCUS,        static_cast<int>(true));
    intmap.insert(WB_CHAT_SHOW_JOINS_FAV,   static_cast<int>(true));
    intmap.insert(WB_CHAT_HIGHLIGHT_FAVS,   static_cast<int>(true));
    intmap.insert(WB_CHAT_ROTATING_MSGS,    static_cast<int>(true));
    intmap.insert(WB_CHAT_USE_SMILE_PANEL,  static_cast<int>(false));
    intmap.insert(WB_CHAT_HIDE_SMILE_PANEL, static_cast<bool>(true));
    intmap.insert(WB_MAINWINDOW_MAXIMIZED,  static_cast<int>(true));
    intmap.insert(WB_MAINWINDOW_HIDE,       static_cast<int>(false));
    intmap.insert(WB_MAINWINDOW_REMEMBER,   static_cast<int>(false));
    intmap.insert(WB_MAINWINDOW_USE_M_TABBAR, static_cast<int>(true));
    intmap.insert(WB_MAINWINDOW_USE_SIDEBAR, static_cast<int>(true));
    intmap.insert(WB_SEARCHFILTER_NOFREE,   static_cast<int>(false));
    intmap.insert(WB_SEARCH_DONTHIDEPANEL,  static_cast<int>(false));
    intmap.insert(WB_ANTISPAM_ENABLED,      static_cast<int>(false));
    intmap.insert(WB_ANTISPAM_AS_FILTER,    static_cast<int>(false));
    intmap.insert(WB_ANTISPAM_FILTER_OPS,   static_cast<int>(false));
    intmap.insert(WB_TRAY_ENABLED,          static_cast<int>(true));
    intmap.insert(WB_EXIT_CONFIRM,          static_cast<int>(false));
    intmap.insert(WB_SHOW_IP_IN_CHAT,       static_cast<int>(false));
    intmap.insert(WB_SHOW_HIDDEN_USERS,     static_cast<int>(false));
    intmap.insert(WB_SHOW_JOINS,            static_cast<int>(false));
    intmap.insert(WB_NOTIFY_ENABLED,        static_cast<int>(true));
    intmap.insert(WB_NOTIFY_SND_ENABLED,    static_cast<int>(false));
    intmap.insert(WB_NOTIFY_SND_EXTERNAL,   static_cast<int>(false));
    intmap.insert(WB_NOTIFY_CH_ICON_ALWAYS, static_cast<int>(false));
    intmap.insert(WB_NOTIFY_SHOW_ON_ACTIVE, static_cast<int>(false));
    intmap.insert(WB_NOTIFY_SHOW_ON_VISIBLE, static_cast<int>(false));
    intmap.insert(WB_FAVUSERS_AUTOGRANT,    static_cast<int>(true));
    intmap.insert(WB_APP_ENABLE_EMOTICON,   static_cast<int>(true));
    intmap.insert(WB_APP_FORCE_EMOTICONS,   static_cast<int>(false));
    intmap.insert(WB_APP_ENABLE_ASPELL,     static_cast<int>(true));
    intmap.insert(WB_APP_REMOVE_NOT_EX_DIRS,static_cast<int>(false));
    intmap.insert(WB_APP_AUTO_AWAY,         static_cast<int>(false));
    intmap.insert(WB_APP_TBAR_SHOW_CL_BTNS, static_cast<int>(true));
    intmap.insert(WB_WIDGETS_PANEL_VISIBLE, static_cast<int>(true));
    intmap.insert(WB_TOOLS_PANEL_VISIBLE,   static_cast<int>(true));
    intmap.insert(WB_SEARCH_PANEL_VISIBLE,  static_cast<int>(false));
    intmap.insert(WB_MAIN_MENU_VISIBLE,     static_cast<int>(true));
    intmap.insert(WB_USE_CTRL_ENTER,        static_cast<int>(false));
    intmap.insert(WB_SIMPLE_SHARE_MODE,     static_cast<int>(false));
    intmap.insert(WI_APP_UNIT_BASE,         1024);
    intmap.insert(WI_APP_AUTOAWAY_INTERVAL, 60);
    intmap.insert(WI_APP_SHARED_FILES_ALPHA, 127);
    intmap.insert(WI_CHAT_MAXPARAGRAPHS,    1000);
    intmap.insert(WI_DEF_MAGNET_ACTION,     0);
    intmap.insert(WI_CHAT_WIDTH,            -1);
    intmap.insert(WI_CHAT_USERLIST_WIDTH,   -1);
    intmap.insert(WI_CHAT_SORT_COLUMN,      0);
    intmap.insert(WI_CHAT_SORT_ORDER,       0);
    intmap.insert(WI_CHAT_DBLCLICK_ACT,     0);//nick in chat
    intmap.insert(WI_CHAT_MDLCLICK_ACT,     1);//browse files
    intmap.insert(WI_CHAT_FIND_COLOR_ALPHA, 127);
    intmap.insert(WI_CHAT_STATUS_HISTORY_SZ,5);
    intmap.insert(WI_CHAT_STATUS_MSG_MAX_LEN, 128);
    intmap.insert(WI_STATUSBAR_HISTORY_SZ,  WI_STATUSBAR_HISTORY_DEFAULT);
    intmap.insert(WI_MAINWINDOW_HEIGHT,     -1);
    intmap.insert(WI_MAINWINDOW_WIDTH,      -1);
    intmap.insert(WI_MAINWINDOW_X,          -1);
    intmap.insert(WI_MAINWINDOW_Y,          -1);
    intmap.insert(WI_SEARCH_SORT_COLUMN,    1);
    intmap.insert(WI_SEARCH_SORT_ORDER,     0);
    intmap.insert(WI_SEARCH_SHARED_ACTION,  0);
    intmap.insert(WI_SEARCH_LAST_TYPE,      0);
    intmap.insert(WI_TRANSFER_HEIGHT,       -1);
    intmap.insert(WI_SHARE_RPANE_WIDTH,     -1);
    intmap.insert(WI_SHARE_WIDTH,           -1);
    intmap.insert(WI_NOTIFY_EVENTMAP,       0x0B);// 0b00001011, (transfer done = off)
    intmap.insert(WI_NOTIFY_MODULE,         1);//default
    intmap.insert(WI_NOTIFY_SNDMAP,         0x0F);// 0b00001111, all events
    intmap.insert(WI_OUT_IN_HIST,           50);//number of output messages in history
}
