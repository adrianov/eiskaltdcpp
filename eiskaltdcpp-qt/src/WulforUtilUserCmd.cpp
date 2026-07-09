/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforUtil.h"
#include "WulforSettings.h"
#include "MainWindow.h"

#include "dcpp/UserCommand.h"

#include <QInputDialog>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QComboBox>
#include <functional>

using namespace dcpp;

bool WulforUtil::getUserCommandParams(const UserCommand& uc, StringMap& params) {

    StringList names;
    string::size_type i = 0, j = 0;
    const string cmd_str = uc.getCommand();

    while((i = cmd_str.find("%[line:", i)) != string::npos) {
        if ((j = cmd_str.find("]", (i += 7))) == string::npos)
            break;

        names.push_back(cmd_str.substr(i, j - i));
        i = j + 1;
    }

    if (names.empty())
        return true;

    QDialog dlg(MainWindow::getInstance());
    dlg.setWindowTitle(_q(uc.getDisplayName().back()));

    QVBoxLayout *vlayout = new QVBoxLayout(&dlg);

    std::vector<std::function<void ()> > valueFs;

    for (const auto &name : names) {
        QString caption = _q(name);

        if (uc.adc()) {
            caption.replace("\\\\", "\\");
            caption.replace("\\s", " ");
        }

        int combo_sel = -1;
        QString combo_caption = caption;
        combo_caption.replace("//", "\t");
        QStringList combo_values = combo_caption.split("/");

        if (combo_values.size() > 2) {
            QString tmp = combo_values.takeFirst();

            bool isNumber = false;
            combo_sel = combo_values.takeFirst().toInt(&isNumber);
            if (!isNumber || combo_sel >= combo_values.size())
                combo_sel = -1;
            else
                caption = tmp;
        }

        QGroupBox *box = new QGroupBox(caption, &dlg);
        QHBoxLayout *hlayout = new QHBoxLayout(box);

        if (combo_sel >= 0) {
            for (auto &val : combo_values)
                val.replace("\t", "/");

            QComboBox *combo = new QComboBox(box);
            hlayout->addWidget(combo);

            combo->addItems(combo_values);
            combo->setEditable(true);
            combo->setCurrentIndex(combo_sel);
            combo->lineEdit()->setReadOnly(true);

            valueFs.push_back([combo, name, &params] {
                params["line:" + name] = combo->currentText().toStdString();
            });

        } else {
            QLineEdit *line = new QLineEdit(box);
            hlayout->addWidget(line);

            valueFs.push_back([line, name, &params] {
                params["line:" + name] = line->text().toStdString();
            });
        }

        vlayout->addWidget(box);
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(&dlg);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    vlayout->addWidget(buttonBox);

    dlg.setFixedHeight(vlayout->sizeHint().height());

    connect(buttonBox, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dlg, SLOT(reject()));

    if (dlg.exec() != QDialog::Accepted)
        return false;

    for (const auto &fs : valueFs)
        fs();

    return true;
}

