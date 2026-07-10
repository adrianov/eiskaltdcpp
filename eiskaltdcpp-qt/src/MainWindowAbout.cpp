/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "MainWindow.h"
#include "MainWindowPrivate.h"
#include "AboutDialog.h"
#include "VersionGlobal.h"
#include "WulforUtil.h"

#include "dcpp/SettingsManager.h"

#include <QAction>
#include <QDesktopServices>
#include <QHash>
#include <QUrl>

using namespace dcpp;

void MainWindow::slotAboutOpenUrl(){
    Q_D(MainWindow);

    QAction *act = qobject_cast<QAction *>(sender());

#if QT_VERSION >= 0x050000
    const QHash<QAction*, QUrl> urlsTable = {
        { d->aboutHomepage,     QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/#description") },
        { d->aboutBuilds,       QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/#packages-and-installers") },
        { d->aboutSource,       QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/") },
        { d->aboutIssues,       QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/issues") },
        { d->aboutWiki,         QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/wiki") },
        { d->aboutChangelog,    QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/blob/master/ChangeLog.txt") },
    };
#else
    QHash<QAction*, QUrl> urlsTable;
    urlsTable.insert( d->aboutHomepage,     QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/#description") );
    urlsTable.insert( d->aboutBuilds,       QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/#packages-and-installers") );
    urlsTable.insert( d->aboutSource,       QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/") );
    urlsTable.insert( d->aboutIssues,       QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/issues") );
    urlsTable.insert( d->aboutWiki,         QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/wiki") );
    urlsTable.insert( d->aboutChangelog,    QUrl("https://github.com/eiskaltdcpp/eiskaltdcpp/blob/master/ChangeLog.txt") );
#endif

    if (urlsTable.contains(act)) {
        QDesktopServices::openUrl(urlsTable[act]);
    }
}

void MainWindow::slotAboutClient() {
    About a(this);

    double ratio;
    double down = static_cast<double>(SETTING(TOTAL_DOWNLOAD));
    double up   = static_cast<double>(SETTING(TOTAL_UPLOAD));

    if (down > 0)
        ratio = up / down;
    else
        ratio = 0;

    a.label->setText(QString("<b>%1</b> %2")
                     .arg(QString::fromStdString(eiskaltdcppAppNameString))
                     .arg(QString::fromStdString(eiskaltdcppVersionString)));

    QString html_format = "a { text-decoration:none; }\n"
                          "a:hover { text-decoration: underline; }\n";

    QString about_text = tr("EiskaltDC++ is a graphical client for Direct Connect and ADC protocols.")+
                         QString("<br/>")+
                         QString("<br/>")+
                         tr("DC++ core version: %1 (modified)").arg(DCPP_VERSION)+
                         QString("<br/>")+
                         QString("<br/>")+
                         tr("Home page: ")+
                         QString("<a href=\"https://github.com/eiskaltdcpp/eiskaltdcpp/\">"
                                 "https://github.com/eiskaltdcpp/eiskaltdcpp/</a>")+
                         QString("<br/>")+
                         QString("<br/>")+
                         tr("Total up: <b>%1</b>").arg(WulforUtil::formatBytes(up))+
                         QString("<br/>")+
                         tr("Total down: <b>%1</b>").arg(WulforUtil::formatBytes(down))+
                         QString("<br/>")+
                         tr("Ratio: <b>%1</b>").arg(ratio, 0, 'f', 3);

    a.label_ABOUT->setText(about_text);

    a.textBrowser_AUTHORS->document()->setDefaultStyleSheet(html_format);

    a.textBrowser_AUTHORS->setText(
        tr("Please use <a href=\"https://github.com/eiskaltdcpp/eiskaltdcpp/issues\">"
        "https://github.com/eiskaltdcpp/eiskaltdcpp/issues</a> to report bugs.<br/>")+
        QString("<br/>")+
        tr("<b>Developers</b><br/>")+
        QString("<br/>")+
        QString("&nbsp; 2009-2012 <a href=\"mailto:dein.negativ@gmail.com\">Andrey Karlov</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("lead developer")+QString(", 2009-2012")+
        QString("<br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("release manager")+QString(", 2009-2010")+
        QString("<br/>")+
        QString("<br/>")+
        QString("&nbsp; 2009-2015 <a href=\"mailto:dhamp@ya.ru\">Eugene Petrov</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("Arch Linux maintainer")+QString(", 2009-2015")+
        QString("<br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("developer")+QString(", 2009-2015")+
        QString("<br/>")+
        QString("<br/>")+
        QString("&nbsp; 2010-2023 <a href=\"mailto:tehnick-8@yandex.ru\">Boris Pek</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("Debian/Ubuntu maintainer")+QString(", 2010-2019")+
        QString("<br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("developer")+QString(", 2010-2019")+
        QString("<br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("translations coordinator")+QString(", 2010-2019")+
        QString("<br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("release manager")+QString(", 2010-2019")+
        QString("<br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("lead developer")+QString(", 2012-2019")+
        QString("<br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("macOS maintainer")+QString(", 2018-2019")+
        QString("<br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("MS Windows maintainer")+QString(", 2019")+
        QString("<br/>")+
        QString("<br/>")+
        QString("&nbsp; 2010-2015 <a href=\"mailto:pavelvat@gmail.com\">Pavel Vatagin</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("MS Windows maintainer")+QString(", 2010-2017")+
        QString("<br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("developer")+QString(", 2010-2017")+
        QString("<br/>")+
        QString("<br/>")+
        QString("&nbsp; 2010-2014 <a href=\"mailto:tka4ev@gmail.com\">Alexandr Tkachev</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("developer")+QString(", 2010-2014")+
        QString("<br/>")+
        QString("<br/>")+
        tr("<b>Graphic files</b><br/>")+
        QString("<br/>")+
        QString("&nbsp; 2009-2010 <a href=\"mailto:wiselord1983@gmail.com\">Uladzimir Bely</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("creator of the logo of the project")+
        QString("<br/>")+
        QString("<br/>")+
        QString("&nbsp; 2010-2015 <a href=\"mailto:tehnick-8@yandex.ru\">Boris Pek</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; * ")+
        tr("tiny updates of the logo")+
        QString("<br/>")+
        QString("<br/>")
        );

    a.textBrowser_TRANSLATION->document()->setDefaultStyleSheet(html_format);

    a.textBrowser_TRANSLATION->setText(
        tr("Participate in the translation. It is easy:")+
        QString("<br/>")+
        QString("<a href=\"https://www.transifex.com/tehnick/eiskaltdcpp/\">https://www.transifex.com/tehnick/eiskaltdcpp/</a><br/>")+
        QString("<br/>")+
        tr("Russian translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2010-2023 <a href=\"mailto:tehnick-8@yandex.ru\">Boris Pek</a> aka Tehnick<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2009-2010 <a href=\"mailto:wiselord1983@gmail.com\">Uladzimir Bely</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2012 <a href=\"mailto:tret2003@gmail.com\">Vyacheslav Tretyakov</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2018 <a href=\"https://app.transifex.com/user/profile/adem4ik/\">Andrei Stepanov</a><br/>")+
        QString("<br/>")+
        tr("Belarusian translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2009-2013 <a href=\"mailto:i.kliok@gmail.com\">Paval Shalamitski</a> aka Klyok<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2015 <a href=\"mailto:m_d@tut.by\">Zmicer Melayok</a><br/>")+
        QString("<br/>")+
        tr("Hungarian translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2010-2012 <a href=\"mailto:husumo@gmail.com\">Akos Berki</a> aka sumo<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2011-2014 <a href=\"mailto:marcus@elitemail.hu\">Márk Lutring</a><br/>")+
        QString("<br/>")+
        tr("French translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2010-2023 <a href=\"mailto:alexandre.wallimann@gmail.com\">Alexandre Wallimann</a> aka Jellyffs<br/>")+
        QString("<br/>")+
        tr("Polish translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2010-2012 <a href=\"mailto:arahael@gmail.com\">Arahael</a><br/>")+
        QString("<br/>")+
        tr("Ukrainian translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2010 <a href=\"mailto:dmytro.demenko@gmail.com\">Dmytro Demenko</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2013-2014 <a href=\"mailto:grayich@ukr.net\">gray</a> aka grayich<br/>")+
        QString("<br/>")+
        tr("Serbian (Cyrillic) translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2014 <a href=\"mailto:trifunovic@openmailbox.org\">Marko Trifunović</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2015 <a href=\"mailto:miroslav031@gmail.com\">Miroslav Petrovic</a><br/>")+
        QString("<br/>")+
        tr("Serbian (Latin) translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2010-2015 <a href=\"mailto:miroslav031@gmail.com\">Miroslav Petrovic</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2014 <a href=\"mailto:trifunovic@openmailbox.org\">Marko Trifunović</a><br/>")+
        QString("<br/>")+
        tr("Spanish translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2010-2019 <a href=\"mailto:sl1pkn07@gmail.com\">Gustavo Alvarez</a> aka sL1pKn07<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2012-2021 <a href=\"mailto:klondike@klondike.es\">Francisco Blas Izquierdo Riera</a> aka klondike<br/>")+
        QString("<br/>")+
        tr("Basque translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2014-2015 <a href=\"mailto:egoitzro2@hotmail.com\">Egoitz Rodriguez</a><br/>")+
        QString("<br/>")+
        tr("Bulgarian translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2010-2012 <a href=\"mailto:dimitrov.rusi@gmail.com\">Rusi Dimitrov</a> aka PsyTrip<br/>")+
        QString("<br/>")+
        tr("Slovak translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2010-2012 <a href=\"mailto:martin.durisin@gmail.com\">Martin Durisin</a><br/>")+
        QString("<br/>")+
        tr("Czech translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2011-2012 <a href=\"mailto:uhlikx@seznam.cz\">Uhlik</a><br/>")+
        QString("<br/>")+
        tr("German translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2011-2012 <a href=\"mailto:kgeorgokitsos@yahoo.de\">Konstantinos Georgokitsos</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2011-2012 <a href=\"mailto:tilkax@gmail.com\">Tillmann Karras</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2012-2016 <a href=\"mailto:be.w@mail.ru\">Benjamin Weber</a><br/>")+
        QString("<br/>")+
        tr("Greek translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2011-2012 <a href=\"mailto:kgeorgokitsos@yahoo.de\">Konstantinos Georgokitsos</a><br/>")+
        QString("<br/>")+
        tr("Italian translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2012 <a href=\"mailto:netcelli@gmail.com\">Stefano Simoncelli</a><br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2012 <a href=\"mailto:lorenzo.keller@gmail.com\">Lorenzo Keller</a><br/>")+
        QString("<br/>")+
        tr("Portuguese (Brazil) translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2013-2015 <a href=\"mailto:heldercro@gmail.com\">Helder Cesar</a> aka redrum<br/>")+
        QString("<br/>")+
        tr("Vietnamese translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2013 <a href=\"mailto:ppanhh@gmail.com\">Anh Phan</a><br/>")+
        QString("<br/>")+
        tr("Chinese (China) translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2013 <a href=\"mailto:syaomingl@gmail.com\">Syaoming Lai</a><br/>")+
        QString("<br/>")+
        tr("Swedish (Sweden) translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2014-2023 <a href=\"mailto:sopor@hotmail.com\">Sopor</a><br/>")+
        QString("<br/>")+
        tr("Turkish translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2015-2023 <a href=\"https://app.transifex.com/user/profile/mauron/\">mauron</a><br/>")+
        QString("<br/>")+
        tr("Danish translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2021 <a href=\"mailto:What2Write@protonmail.com\">What2Write</a><br/>")+
        QString("<br/>")+
        tr("Georgian translation<br/>")+
        QString("&nbsp;&nbsp;&nbsp; 2023 <a href=\"mailto:temuri.doghonadze@gmail.com\">Temuri Doghonadze</a><br/>")+
        QString("<br/>")
        );

    a.textBrowser_LICENSE->document()->setDefaultStyleSheet(html_format);

    a.textBrowser_LICENSE->setText(
        QString("This program is free software: you can redistribute it and/or modify it under the terms "
                "of the GNU General Public License as published by the Free Software Foundation, either "
                "version 3 of the License, or (at your option) any later version.<br/>"
                "<br/>"
                "This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; "
                "without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. "
                "See the GNU General Public License for more details.<br/>"
                "<br/>"
                "You should have received a copy of the GNU General Public License along with this program. "
                "If not, see &lt;<a href=\"https://www.gnu.org/licenses/\">https://www.gnu.org/licenses/</a>&gt;.<br/>")
        );

    a.exec();
}

