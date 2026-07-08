/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ChatSearchBar.h"

#include "AppTheme.h"
#include "LineEdit.h"
#include "WulforSettings.h"

#include <QColor>
#include <QFrame>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextEdit>
#include <QToolButton>

ChatSearchBar::ChatSearchBar(QFrame *searchFrame, LineEdit *findEdit, QToolButton *backButton,
                              QToolButton *forwardButton, QToolButton *allButton_, QToolButton *hideButton,
                              QTextEdit *chatEdit, QObject *parent) :
    QObject(parent), frame(searchFrame), edit(findEdit), allButton(allButton_), chat(chatEdit)
{
    frame->setVisible(false);

    connect(backButton, SIGNAL(clicked()), this, SLOT(findBackward()));
    connect(forwardButton, SIGNAL(clicked()), this, SLOT(findForward()));
    connect(hideButton, SIGNAL(clicked()), this, SLOT(dismiss()));
    connect(allButton, SIGNAL(clicked()), this, SLOT(findAll()));
    connect(edit, SIGNAL(textEdited(QString)), this, SLOT(textChanged(QString)));
}

void ChatSearchBar::activate(){
    frame->setVisible(true);

    const QString selected = chat->textCursor().selectedText();
    if (!selected.isEmpty())
        edit->setText(selected);

    edit->selectAll();
    edit->setFocus();
}

void ChatSearchBar::dismiss(){
    frame->setVisible(false);

    QTextCursor c = chat->textCursor();
    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor, 1);
    chat->setExtraSelections(QList<QTextEdit::ExtraSelection>());
    chat->setTextCursor(c);
}

void ChatSearchBar::findForward(){ findText(QTextDocument::FindFlags()); }
void ChatSearchBar::findBackward(){ findText(QTextDocument::FindBackward); }

void ChatSearchBar::findText(QTextDocument::FindFlags flag){
    chat->setExtraSelections(QList<QTextEdit::ExtraSelection>());

    if (edit->text().isEmpty())
        return;

    QTextCursor c = chat->textCursor();
    const bool ok = chat->find(edit->text(), flag);

    if (flag == QTextDocument::FindBackward && !ok)
        c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 1);
    else if (!flag && !ok)
        c.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1);

    c = chat->document()->find(edit->text(), c, flag);
    if (!c.isNull()){
        chat->setTextCursor(c);
        findAll();
    }
}

void ChatSearchBar::textChanged(const QString &text){
    if (text.isEmpty()){
        chat->verticalScrollBar()->setValue(chat->verticalScrollBar()->maximum());
        QTextCursor c = chat->textCursor();
        c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 1);
        chat->setTextCursor(c);

        return;
    }

    QTextCursor c = chat->textCursor();

    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor, 1);
    c = chat->document()->find(edit->text(), c, {});
    if (!c.isNull()){
        chat->setExtraSelections(QList<QTextEdit::ExtraSelection>());
        chat->setTextCursor(c);
        findAll();
    }
}

void ChatSearchBar::findAll(){
    if (!allButton->isChecked()){
        chat->setExtraSelections(QList<QTextEdit::ExtraSelection>());

        return;
    }

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!edit->text().isEmpty()){
        QTextEdit::ExtraSelection selection;

        QColor color;
        color.setNamedColor(AppTheme::chatColor(WS_CHAT_FIND_COLOR));
        color.setAlpha(WIGET(WI_CHAT_FIND_COLOR_ALPHA));

        selection.format.setBackground(color);

        QTextCursor c = chat->document()->find(edit->text(), 0, {});

        while (!c.isNull()){
            selection.cursor = c;
            extraSelections.append(selection);

            c = chat->document()->find(edit->text(), c, {});
        }
    }

    chat->setExtraSelections(extraSelections);
}
