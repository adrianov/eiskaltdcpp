/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ChatEdit.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QKeyEvent>
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>

#include "AppTheme.h"

ChatEdit::ChatEdit(QWidget *parent) : QTextEdit(parent)
{
    setMinimumHeight(10);
    setFrameShape(QFrame::NoFrame);
    setAutoFillBackground(true);
    AppTheme::applyInputPalette(this);

    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(this, SIGNAL(textChanged()), this, SLOT(recalculateGeometry()));
}

ChatEdit::~ChatEdit()
{}

QSize ChatEdit::minimumSizeHint() const{
    QSize sh = QTextEdit::minimumSizeHint();
    sh.setHeight(fontMetrics().height() + 1);
    sh += QSize(0, QFrame::lineWidth() * 2);
    return sh;
}

QSize ChatEdit::sizeHint() const{
    QSize sh = QTextEdit::sizeHint();
    sh.setHeight(int(document()->documentLayout()->documentSize().height()));
    sh += QSize(0, QFrame::lineWidth() * 2);
    return sh.expandedTo(minimumSizeHint());
}

void ChatEdit::paintEvent(QPaintEvent *e)
{
    QTextEdit::paintEvent(e);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(AppTheme::inputBorder(hasFocus()), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 5, 5);
}

void ChatEdit::focusOutEvent(QFocusEvent *e)
{
    QTextEdit::focusOutEvent(e);
    update();
}

void ChatEdit::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::PaletteChange || e->type() == QEvent::ThemeChange) {
        AppTheme::applyInputPalette(this);
        update();
    }

    QTextEdit::changeEvent(e);
}

void ChatEdit::keyPressEvent(QKeyEvent *e)
{
    const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    bool hasModifier = (e->modifiers() != Qt::NoModifier) &&
                       (e->modifiers() != Qt::KeypadModifier) &&
                       !ctrlOrShift;

    if (e->key() == Qt::Key_Tab) {
        if (!toPlainText().isEmpty()) {
            if (cc && cc->popup()->isVisible()) {
                int row = cc->popup()->currentIndex().row() + 1;
                if (cc->completionModel()->rowCount() == row)
                    row = 0;
                cc->popup()->setCurrentIndex(cc->completionModel()->index(row, 0));
            }
            e->accept();
        } else {
            e->ignore();
        }
        return;
    }

    if (cc && cc->popup()->isVisible()) {
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Backtab:
            e->ignore();
            return;
        default:
            break;
        }
    }

    if (!cc || !cc->popup()->isVisible() || !hasModifier)
        QTextEdit::keyPressEvent(e);

    if (ctrlOrShift && e->text().isEmpty())
        return;

    if (!cc)
        return;

    if (cc->popup()->isVisible() && (hasModifier || e->text().isEmpty())) {
        cc->popup()->hide();
        return;
    }

    if (cc->popup()->isVisible())
        complete();
}

void ChatEdit::keyReleaseEvent(QKeyEvent *e)
{
    bool hasModifier = (e->modifiers() != Qt::NoModifier);

    switch (e->key()) {
    case Qt::Key_Tab:
        if (cc && !hasModifier && !cc->popup()->isVisible())
            complete();

    case Qt::Key_Enter:
    case Qt::Key_Return:
        e->ignore();
        return;
    default:
        break;
    }
}

void ChatEdit::updateScrollBar(){
    setVerticalScrollBarPolicy(sizeHint().height() > height() ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
    ensureCursorVisible();
}
