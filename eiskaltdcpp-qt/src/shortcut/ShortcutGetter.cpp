/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "shortcut/ShortcutGetter.h"
#include "QtCompat.h"

#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>
#include <QPushButton>
#include <QDialogButtonBox>

ShortcutGetter::ShortcutGetter(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Modify shortcut"));

    QVBoxLayout *vbox = new QVBoxLayout(this);
    wulforSetMargin(vbox, 2);
    vbox->setSpacing(4);

    QLabel *l = new QLabel(this);
    l->setText(tr("Press the key combination you want to assign"));
    vbox->addWidget(l);

    leKey = new QLineEdit(this);
    leKey->installEventFilter(this);
    vbox->addWidget(leKey);

    setCaptureKeyboard(true);
    QDialogButtonBox *buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel |
                                                       QDialogButtonBox::Reset);
    QPushButton *clearbutton = buttonbox->button(QDialogButtonBox::Reset);
    clearbutton->setText(tr("Clear"));

    QPushButton *captureButton = new QPushButton(tr("Capture"), this);
    captureButton->setToolTip(tr("Capture keystrokes"));
    captureButton->setCheckable(captureKeyboard());
    captureButton->setChecked(captureKeyboard());
    connect(captureButton, SIGNAL(toggled(bool)), this, SLOT(setCaptureKeyboard(bool)));

    buttonbox->addButton(captureButton, QDialogButtonBox::ActionRole);
    connect(buttonbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonbox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(clearbutton, SIGNAL(clicked()), leKey, SLOT(clear()));
    vbox->addWidget(buttonbox);
}

void ShortcutGetter::setCaptureKeyboard(bool b) {
    capture = b;
    leKey->setReadOnly(b);
    leKey->setFocus();
}

QString ShortcutGetter::exec(const QString& s)
{
    chord.arm();
    leKey->setText(s);

    if (QDialog::exec() == QDialog::Accepted)
        return leKey->text();
    return QString();
}

bool ShortcutGetter::event(QEvent *e)
{
    if (!capture)
        return QDialog::event(e);

    switch (e->type()) {
    case QEvent::KeyPress: {
        const QKeyEvent *k = static_cast<QKeyEvent*>(e);
        chord.press(k->key(), k->modifiers(), k->text());
        leKey->setText(chord.text());
        return true;
    }
    case QEvent::KeyRelease:
        chord.release();
        return true;
    default:
        return QDialog::event(e);
    }
}

bool ShortcutGetter::eventFilter(QObject *o, QEvent *e)
{
    if (!capture)
        return QDialog::eventFilter(o, e);
    if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease)
        return event(e);
    return QDialog::eventFilter(o, e);
}
