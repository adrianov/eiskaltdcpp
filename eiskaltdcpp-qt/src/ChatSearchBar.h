/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QObject>
#include <QTextDocument>

class QFrame;
class QTextEdit;
class QToolButton;
class LineEdit;

/**
 * Owns the inline "Find in chat" bar: showing/hiding it, stepping through
 * matches, live-highlighting while typing, and highlighting all occurrences.
 */
class ChatSearchBar : public QObject
{
    Q_OBJECT

public:
    ChatSearchBar(QFrame *searchFrame, LineEdit *findEdit, QToolButton *backButton,
                  QToolButton *forwardButton, QToolButton *allButton, QToolButton *hideButton,
                  QTextEdit *chatEdit, QObject *parent);

    void activate();

public Q_SLOTS:
    void dismiss();
    void findForward();
    void findBackward();
    void findAll();
    void textChanged(const QString &text);

private:
    void findText(QTextDocument::FindFlags flag);

    QFrame *frame;
    LineEdit *edit;
    QToolButton *allButton;
    QTextEdit *chat;
};
