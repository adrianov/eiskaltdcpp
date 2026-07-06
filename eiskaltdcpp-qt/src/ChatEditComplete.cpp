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
#include <QFocusEvent>
#include <QScrollBar>
#include <QTextBlock>

void ChatEdit::setCompleter(QCompleter *completer, UserListModel *model)
{
    if (cc)
        QObject::disconnect(cc, nullptr, this, nullptr);

    cc = completer;
    cc_model = model;

    if (!cc || !model)
        return;

    cc->setWidget(this);
    cc->setWrapAround(false);
    cc->setCaseSensitivity(Qt::CaseInsensitive);
    cc->setCompletionMode(QCompleter::PopupCompletion);

    QObject::connect(cc, SIGNAL(activated(const QModelIndex&)),
                     this, SLOT(insertCompletion(const QModelIndex&)));
}

void ChatEdit::insertCompletion(const QModelIndex & index)
{
    if (!cc || cc->widget() != this || !index.isValid())
        return;

    QString nick = cc->completionModel()->index(index.row(), index.column()).data().toString();
    int begin = textCursor().position() - cc->completionPrefix().length();

    insertToPos(nick, begin);
}

void ChatEdit::insertToPos(const QString & completeText, int begin)
{
    if (completeText.isEmpty())
        return;

    if (begin < 0)
        begin = 0;

    QTextCursor cursor = textCursor();
    int end = cursor.position();
    cursor.setPosition(begin);
    cursor.setPosition(end, QTextCursor::KeepAnchor);

    if (!begin)
        cursor.insertText(completeText + ": ");
    else
        cursor.insertText(completeText + " ");

    setTextCursor(cursor);
}

QString ChatEdit::textUnderCursor() const
{
    QTextCursor cursor = textCursor();

    int curpos = cursor.position();
    QString text = cursor.block().text().left(curpos);

    QStringList wordList = text.split(QRegExp("\\s"));

    if (wordList.isEmpty())
        return QString();

    return wordList.last();
}

void ChatEdit::focusInEvent(QFocusEvent *e)
{
    if (cc)
        cc->setWidget(this);

    QTextEdit::focusInEvent(e);
}

void ChatEdit::complete()
{
    if (!cc || !cc_model)
        return;

    QString completionPrefix = textUnderCursor();

    if (completionPrefix.isEmpty()) {
        if (cc->popup()->isVisible())
            cc->popup()->hide();

        return;
    }

    if (!cc->popup()->isVisible() || completionPrefix.length() < cc->completionPrefix().length()) {
        QString pattern = QString("(\\[.*\\])?%1.*").arg( QRegExp::escape(completionPrefix) );
        QStringList nicks = cc_model->findItems(pattern, Qt::MatchRegExp, 0);

        if (nicks.isEmpty())
            return;

        if (nicks.count() == 1) {
            insertToPos(nicks.last(), textCursor().position() - completionPrefix.length());
            return;
        }

        NickCompletionModel *tmpModel = new NickCompletionModel(nicks, cc);
        cc->setModel(tmpModel);
    }

    if (completionPrefix != cc->completionPrefix()) {
        cc->setCompletionPrefix(completionPrefix);
        cc->popup()->setCurrentIndex(cc->completionModel()->index(0, 0));
    }

    QRect cr = cursorRect();
    cr.setWidth(cc->popup()->sizeHintForColumn(0)
                + cc->popup()->verticalScrollBar()->sizeHint().width());

    cc->complete(cr);
}
