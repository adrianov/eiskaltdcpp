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

#ifndef CLIENT_ICONS_DIR
#define CLIENT_ICONS_DIR ""
#endif

SettingsGUI::SettingsGUI(QWidget *parent) :
    QWidget(parent),
    custom_style(false)
{
    setupUi(this);

    init();
}

SettingsGUI::~SettingsGUI(){

}

void SettingsGUI::slotTestAppTheme(){
    custom_style = true;

    if (!comboBox_THEMES->currentIndex()) {
        WSSET(WS_APP_THEME, "");
        AppTheme::applyPreferredStyle();
        AppTheme::apply();
        return;
    }

    QString s = comboBox_THEMES->currentText();
    if (s.isEmpty())
        return;

    qApp->setStyle(s);
    WSSET(WS_APP_THEME, s);
    AppTheme::apply();
}

void SettingsGUI::slotThemeChanged(){
    custom_style = true;
}

void SettingsGUI::slotBrowseLng(){
    QString file = QFileDialog::getOpenFileName(this,
                                                tr("Select translation"),
                                                WulforUtil::getInstance()->getTranslationsPath(),
                                                tr("Translation (*.qm)"));

    if (!file.isEmpty()){
        file = QDir::toNativeSeparators(file);

        WulforSettings::getInstance()->blockSignals(true);//do not emit signal that translation file has been changed
        WSSET(WS_TRANSLATION_FILE, file);
        WulforSettings::getInstance()->blockSignals(false);

        WulforSettings::getInstance()->loadTranslation();//set language for application
        MainWindow::getInstance()->retranslateUi();

        WSSET(WS_TRANSLATION_FILE, file);//emit signals for other widgets

        lineEdit_LANGFILE->setText(WSGET(WS_TRANSLATION_FILE));
    }
}

void SettingsGUI::slotLngIndexChanged(int index){
    QString file = comboBox_LANGS->itemData(index).toString();

    WSSET(WS_TRANSLATION_FILE, file);

    WulforSettings::getInstance()->blockSignals(true);//do not emit signal that translation file has been changed
    WSSET(WS_TRANSLATION_FILE, file);
    WulforSettings::getInstance()->blockSignals(false);

    WulforSettings::getInstance()->loadTranslation();
    MainWindow::getInstance()->retranslateUi();

    WSSET(WS_TRANSLATION_FILE, file);

    lineEdit_LANGFILE->setText(WSGET(WS_TRANSLATION_FILE));
}

void SettingsGUI::slotIconsChanged(){
    WSSET(WS_APP_ICONTHEME, comboBox_ICONS->currentText());

    WulforUtil::getInstance()->loadIcons();
}

void SettingsGUI::slotUsersChanged(){
    WSSET(WS_APP_USERTHEME, comboBox_USERS->currentText());
}
