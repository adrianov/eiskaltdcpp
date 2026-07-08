/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "LineEdit.h"
#include "AppTheme.h"
#include "WulforUtil.h"

#include <QMouseEvent>
#include <QPainter>

static const int margin = 3;

LineEdit::LineEdit(QWidget *parent) :
        QLineEdit(parent), menu(nullptr), role(LineEdit::InsertText)
{
    pxm = WICON_SIZE(WulforUtil::eiEDITCLEAR, 16);

    setFrame(false);
    setAutoFillBackground(true);

    label = new QLabel(this);
    label->setPixmap(pxm);
    label->setCursor(Qt::ArrowCursor);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    label->installEventFilter(this);

    connect(this, SIGNAL(textChanged(QString)), this, SLOT(slotTextChanged()));

    AppTheme::applyInputPalette(this);
    updateGeometry();
    updateStyles();
    slotTextChanged();
}

LineEdit::~LineEdit(){
    label->deleteLater();

    if (menu)
        menu->deleteLater();
}

void LineEdit::resizeEvent(QResizeEvent *e){
    e->accept();
    updateGeometry();
}

void LineEdit::paintEvent(QPaintEvent *e){
    QLineEdit::paintEvent(e);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor border = AppTheme::inputBorder(hasFocus());

    painter.setPen(QPen(border, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 5, 5);
}

void LineEdit::focusInEvent(QFocusEvent *e){
    QLineEdit::focusInEvent(e);
    update();
}

void LineEdit::focusOutEvent(QFocusEvent *e){
    QLineEdit::focusOutEvent(e);
    update();
}

bool LineEdit::eventFilter(QObject *obj, QEvent *e){
    if (qobject_cast<QLabel*>(obj) != label)
        return QLineEdit::eventFilter(obj, e);

    switch (e->type()){
    case QEvent::MouseButtonPress:
        {
            if (!menu)
                clear();
            else{
                QAction *act = menu->exec(mapToGlobal(label->pos()+QPoint(0, label->size().height())));

                if (act && role == InsertText)
                    setText(act->text());
                else if (act)
                    emit menuAction(act);
            }

            break;
        }
    default:
        break;
    }

    return QLineEdit::eventFilter(obj, e);
}

void LineEdit::updateGeometry(){
    label->setGeometry(width()-pxm.width()-margin*2, 0, pxm.width()+margin, height());
}

void LineEdit::updateStyles(){
    label->setStyleSheet(QString("QLabel { margin-left: %1; }").arg(margin));
    const int right = label->isVisible() ? label->width() + margin : 0;
    setTextMargins(0, 0, right, 0);
}

void LineEdit::changeEvent(QEvent *e){
    if (e->type() == QEvent::PaletteChange || e->type() == QEvent::ThemeChange) {
        AppTheme::applyInputPalette(this);
        update();
    }

    QLineEdit::changeEvent(e);
}

void LineEdit::setPixmap(const QPixmap &px){
    pxm = px;
    label->setPixmap(px);
    updateGeometry();
    updateStyles();
}

void LineEdit::setMenu(QMenu *m){
    if (menu)
        menu->deleteLater();

    menu = m;

    if (menu){
        menu->setParent(nullptr);
        slotTextChanged();
    }
}

void LineEdit::setMenuRole(LineEdit::MenuRole r){
    role = r;
}

void LineEdit::slotTextChanged(){
    if (menu || !text().isEmpty()){
        label->show();
        updateGeometry();
    }
    else
        label->hide();

    updateStyles();
}
