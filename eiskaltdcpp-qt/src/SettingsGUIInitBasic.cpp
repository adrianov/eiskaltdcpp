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

void SettingsGUI::initBasic()
{
    {//Basic tab
        WulforUtil *WU = WulforUtil::getInstance();
        QStringList styles = QStyleFactory::keys();

        comboBox_THEMES->addItem(tr("Default (Fusion)"));

        for (const QString &s : styles)
            comboBox_THEMES->addItem(s);

        comboBox_THEMES->setCurrentIndex(styles.indexOf(WSGET(WS_APP_THEME)) >= 0? (styles.indexOf(WSGET(WS_APP_THEME))+1) : 0);

        int i = 0;
        int k = -1;

        QDir translationsDir(WU->getTranslationsPath());

        const QMap<QString, QString> langNames ({
            { "en.qm",       tr("English") },
            { "ru.qm",       tr("Russian") },
            { "be.qm",       tr("Belarusian") },
            { "hu.qm",       tr("Hungarian") },
            { "fr.qm",       tr("French") },
            { "pl.qm",       tr("Polish") },
            { "pt_BR.qm",    tr("Portuguese (Brazil)") },
            { "sr.qm",       tr("Serbian (Cyrillic)") },
            { "sr@latin.qm", tr("Serbian (Latin)") },
            { "uk.qm",       tr("Ukrainian") },
            { "es.qm",       tr("Spanish") },
            { "eu.qm",       tr("Basque") },
            { "bg.qm",       tr("Bulgarian") },
            { "sk.qm",       tr("Slovak") },
            { "cs.qm",       tr("Czech") },
            { "de.qm",       tr("German") },
            { "el.qm",       tr("Greek") },
            { "it.qm",       tr("Italian") },
            { "vi.qm",       tr("Vietnamese") },
            { "zh_CN.qm",    tr("Chinese (China)") },
            { "sv_SE.qm",    tr("Swedish (Sweden)") },
            { "tr.qm",       tr("Turkish") },
            { "da.qm",       tr("Danish") },
            { "ka.qm",       tr("Georgian") },
        });

        QString full_path;
        QString lang;

        for (const auto &f : translationsDir.entryList(QDir::Files | QDir::NoSymLinks)){
            full_path = QDir::toNativeSeparators( translationsDir.filePath(f) );
            lang = langNames[f];

            if (!lang.isEmpty()){
                comboBox_LANGS->addItem(lang, full_path);

                if (WSGET(WS_TRANSLATION_FILE).endsWith(f))
                    k = i;

                ++i;
            }
        }
        comboBox_LANGS->setCurrentIndex(k);

        const QString users = WU->getClientIconsPath() + "/user/";
        i = 0;
        k = -1;
        for (const QString &f : QDir(users).entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot)){
            if (!f.isEmpty()){
                comboBox_USERS->addItem(f);

                if (f == WSGET(WS_APP_USERTHEME))
                    k = i;

                ++i;
            }
        }
        comboBox_USERS->setCurrentIndex(k);

        const QString icons = WU->getClientIconsPath() + "/appl/";
        i = 0;
        k = -1;
        for (const QString &f : QDir(icons).entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot)){
            if (!f.isEmpty()){
                comboBox_ICONS->addItem(f);

                if (f == WSGET(WS_APP_ICONTHEME))
                    k = i;

                ++i;
            }
        }
        comboBox_ICONS->setCurrentIndex(k);

        comboBox_EMOT->setCurrentIndex(0);
        i = 0;
        for (const QString &f : QDir(WU->getEmoticonsPath())
             .entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot)){
            if (!f.isEmpty()){
                comboBox_EMOT->addItem(f);

                if (f == WSGET(WS_APP_EMOTICON_THEME))
                    comboBox_EMOT->setCurrentIndex(i);

                ++i;
            }
        }

        lineEdit_LANGFILE->setText(WSGET(WS_TRANSLATION_FILE));

        toolButton_LANGBROWSE->setIcon(WU->getPixmap(WulforUtil::eiFOLDER_BLUE));

        if (WBGET(WB_MAINWINDOW_REMEMBER))
            radioButton_REMEMBER->setChecked(true);
        else if (WBGET(WB_MAINWINDOW_HIDE))
            radioButton_HIDE->setChecked(true);
        else
            radioButton_SHOW->setChecked(true);

        groupBox_TRAY->setChecked(WBGET(WB_TRAY_ENABLED));
        groupBox_TRAY->setEnabled(QSystemTrayIcon::isSystemTrayAvailable());

        if (WBGET(WB_MAINWINDOW_USE_SIDEBAR))
            comboBox_TABBAR->setCurrentIndex(2);
        else if (WBGET(WB_MAINWINDOW_USE_M_TABBAR))
            comboBox_TABBAR->setCurrentIndex(1);
        else
            comboBox_TABBAR->setCurrentIndex(0);

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        checkBox_ICONTHEME->setChecked(WBGET("app/use-icon-theme", false));
#endif
        checkBox_HIDE_ICONS_IN_MENU->setChecked(WBGET("mainwindow/dont-show-icons-in-menus", false));

        // Hide options which do not work in Mac OS X, MS Windows or Haiku:
#if defined (Q_OS_WIN) || defined(Q_OS_MAC) || defined (Q_OS_HAIKU)
        checkBox_ICONTHEME->hide();
#endif
#if defined(Q_OS_MAC)
        groupBox_TRAY->hide();
#endif
    }
}
