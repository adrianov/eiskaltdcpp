/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "PMWindow.h"
#include "WulforSettings.h"
#include "WulforUtil.h"
#include "HubManager.h"

#include <QTextBlock>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QTextEdit>

bool PMWindow::eventFilter(QObject *obj, QEvent *e){
    if (e->type() == QEvent::KeyRelease){
        QKeyEvent *k_e = reinterpret_cast<QKeyEvent*>(e);

        if ((static_cast<QTextEdit*>(obj) == plainTextEdit_INPUT) &&
            (k_e->key() == Qt::Key_Enter || k_e->key() == Qt::Key_Return) &&
            (k_e->modifiers() == Qt::NoModifier))
        {
            return true;
        }
        else if (static_cast<LineEdit*>(obj) == lineEdit_FIND && k_e->key() == Qt::Key_Escape){
            lineEdit_FIND->clear();
            slotHideSearchBar();
            return true;
        }
    }
    else if (e->type() == QEvent::KeyPress){
        QKeyEvent *k_e = reinterpret_cast<QKeyEvent*>(e);

        const bool controlModifier = (k_e->modifiers() == Qt::ControlModifier);

        if (static_cast<QTextEdit*>(obj) == plainTextEdit_INPUT)
        {
            const bool useCtrlEnter = WBGET(WB_USE_CTRL_ENTER);
            const bool keyEnter = (k_e->key() == Qt::Key_Enter || k_e->key() == Qt::Key_Return);
            const bool shiftModifier = (k_e->modifiers() == Qt::ShiftModifier);

            if ((useCtrlEnter && keyEnter && controlModifier) ||
                (!useCtrlEnter && keyEnter && !controlModifier && !shiftModifier))
            {
                const QString msg = plainTextEdit_INPUT->toPlainText();

                HubFrame *fr = qobject_cast<HubFrame*>(HubManager::getInstance()->getHub(hubUrl));

                if (fr) {
                    if (!fr->parseForCmd(msg, this))
                        sendMessage(msg, false, false);
                }
                else {
                    sendMessage(msg, false, false);
                }

                plainTextEdit_INPUT->setPlainText("");

                return true;
            }
        }

        if (controlModifier){
            if (k_e->key() == Qt::Key_Equal || k_e->key() == Qt::Key_Plus){
                textEdit_CHAT->zoomIn();

                return true;
            }
            else if (k_e->key() == Qt::Key_Minus){
                textEdit_CHAT->zoomOut();

                return true;
            }
        }
    }
    else if (e->type() == QEvent::MouseButtonRelease){
        QMouseEvent *m_e = reinterpret_cast<QMouseEvent*>(e);

        if ((static_cast<QWidget*>(obj) == textEdit_CHAT->viewport()) && (m_e->button() == Qt::LeftButton)){
            QString pressedParagraph = textEdit_CHAT->anchorAt(textEdit_CHAT->mapFromGlobal(QCursor::pos()));

            WulforUtil::getInstance()->openUrl(pressedParagraph);
        }
    }
    else if (e->type() == QEvent::MouseMove && (static_cast<QWidget*>(obj) == textEdit_CHAT->viewport())){
        QString str = textEdit_CHAT->anchorAt(textEdit_CHAT->mapFromGlobal(QCursor::pos()));

        if (!str.isEmpty())
            textEdit_CHAT->viewport()->setCursor(Qt::PointingHandCursor);
        else
            textEdit_CHAT->viewport()->setCursor(Qt::IBeamCursor);
    }
    else if (e->type() == QEvent::MouseButtonDblClick){
        HubFrame *fr = qobject_cast<HubFrame*>(HubManager::getInstance()->getHub(hubUrl));
        bool cursoratnick = false;
        QString nick = "",nickstatus="",nickmessage="";
        QString cid = "";
        QTextCursor cursor = textEdit_CHAT->textCursor();

        QString pressedParagraph = cursor.block().text();
        int positionCursor = cursor.columnNumber();
        int l = pressedParagraph.indexOf(" <");
        int r = pressedParagraph.indexOf("> ");
        if (l < r)
            nickmessage = pressedParagraph.mid(l+2, r-l-2);
        else {
            int l1 = pressedParagraph.indexOf(" * ");
            if (l1 > -1 ) {
                QString pressedParagraphstatus = pressedParagraph.remove(0,l1+3).simplified();
                int r1 = pressedParagraphstatus.indexOf(" ");
                nickstatus = pressedParagraphstatus.mid(0, r1);
            }
        }
        if ((!nickmessage.isEmpty() || !nickstatus.isEmpty())&& fr){
            nick = nickmessage + nickstatus;
            cid = fr->getCIDforNick(nick);
        }
        if ((positionCursor < r) && (positionCursor > l))
            cursoratnick = true;

        if (!cid.isEmpty()){
            if (WIGET(WI_CHAT_DBLCLICK_ACT) == 1 && fr && cursoratnick)
                    fr->browseUserFiles(cid, false);
            else if (WIGET(WI_CHAT_DBLCLICK_ACT) == 2 && fr && cursoratnick)
                    fr->addPM(cid, "");
            else if (textEdit_CHAT->anchorAt(textEdit_CHAT->mapFromGlobal(QCursor::pos())).startsWith("user://")){
                if(!plainTextEdit_INPUT->textCursor().position())
                    plainTextEdit_INPUT->textCursor().insertText(nick + WSGET(WS_CHAT_SEPARATOR) + " ");
                else
                    plainTextEdit_INPUT->textCursor().insertText(nick + " ");

                plainTextEdit_INPUT->setFocus();
            }
        }
    }

    return QWidget::eventFilter(obj, e);
}
