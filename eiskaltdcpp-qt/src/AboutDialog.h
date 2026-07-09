/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QDialog>
#include <QCoreApplication>
#include <cstdio>

#include "dcpp/version.h"
#include "ui_UIAbout.h"

class About : public QDialog, public Ui::UIAbout
{
public:
    About(QWidget *parent = nullptr) : QDialog(parent) { setupUi(this); }

    void printHelp() const
    {
        const QString msg = QCoreApplication::translate(
            "About",
            "Usage:\n"
            "  eiskaltdcpp-qt <magnet link> <dchub://link> <adc(s)://link>\n"
            "  eiskaltdcpp-qt <Key>\n"
            "EiskaltDC++ is a cross-platform program that uses the Direct Connect and ADC protocols.\n"
            "\n"
            "Keys:\n"
            "  -h, --help\t Show this message\n"
            "  -V, --version\t Show version string\n"
            "  --share-index-smoke\t Run ShareIndex SQLite self-check and exit\n"
            "  --share-index-reingest <list>\t Force re-index one file list and print ms");
        printf("%s\n", msg.toUtf8().constData());
    }

    void printVersion() const
    {
        printf("%s\n", eiskaltdcppVersionString.c_str());
    }
};
