/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * Hub chat input composition: outgoing message history (Up/Down) and smile panel.
 */

#include "hubframe/HubChatCompose.h"

#include "HubFrame.h"
#include "EmoticonDialog.h"
#include "EmoticonFactory.h"
#include "EmoticonObject.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include <QCursor>
#include <QDir>
#include <QLayout>
#include <QLayoutItem>
#include <QMenu>
#include <QSize>

HubChatCompose::HubChatCompose(HubFrame *hub) : hub(hub)
{
}

void HubChatCompose::remember(const QString &msg)
{
    if (unsent) {
        messages.removeLast();
        unsent = false;
    }

    messages << msg;

    if (messages.size() > WIGET(WI_OUT_IN_HIST))
        messages.removeFirst();

    index = messages.size() - 1;
}

void HubChatCompose::nextMsg()
{
    if (!hub->plainTextEdit_INPUT->hasFocus())
        return;

    if (index < 0 || index + 1 > messages.size() - 1 || messages.isEmpty())
        return;

    if (messages.at(index) != hub->plainTextEdit_INPUT->toPlainText())
        messages[index] = hub->plainTextEdit_INPUT->toPlainText();

    if (index + 1 <= messages.size() - 1)
        index++;

    hub->plainTextEdit_INPUT->setPlainText(messages.at(index));

    if (unsent && index == messages.size() - 1) {
        messages.removeLast();
        unsent = false;
        index = messages.size() - 1;
    }
}

void HubChatCompose::prevMsg()
{
    if (!hub->plainTextEdit_INPUT->hasFocus())
        return;

    if (index < 1 || index - 1 > messages.size() - 1 || messages.isEmpty())
        return;

    if (!unsent && index == messages.size() - 1) {
        messages << hub->plainTextEdit_INPUT->toPlainText();
        unsent = true;
        index++;
    }

    if (messages.at(index) != hub->plainTextEdit_INPUT->toPlainText())
        messages[index] = hub->plainTextEdit_INPUT->toPlainText();

    if (index >= 1)
        index--;

    hub->plainTextEdit_INPUT->setPlainText(messages.at(index));
}

void HubChatCompose::insertSmile(QString smiley)
{
    if (smiley.isEmpty())
        return;

    smiley.replace("&lt;", "<");
    smiley.replace("&gt;", ">");
    smiley.replace("&amp;", "&");
    smiley.replace("&apos;", "\'");
    smiley.replace("&quot;", "\"");
    smiley += " ";

    hub->plainTextEdit_INPUT->textCursor().insertText(smiley);
    hub->plainTextEdit_INPUT->setFocus();
}

void HubChatCompose::toggleSmiles()
{
    if (!(WBGET(WB_APP_ENABLE_EMOTICON) && EmoticonFactory::getInstance()))
        return;

    if (WBGET(WB_CHAT_USE_SMILE_PANEL)) {
        hub->frame_SMILES->setVisible(!hub->frame_SMILES->isVisible());
        return;
    }

    EmoticonDialog *dialog = new EmoticonDialog(hub);

    if (dialog->exec() == QDialog::Accepted)
        insertSmile(dialog->getEmoticonText());

    delete dialog;
}

void HubChatCompose::smileClicked(QObject *sender)
{
    EmoticonLabel *lbl = qobject_cast<EmoticonLabel *>(sender);

    if (!lbl)
        return;

    insertSmile(lbl->toolTip());

    if (WBGET(WB_CHAT_HIDE_SMILE_PANEL))
        hub->frame_SMILES->setVisible(false);
}

void HubChatCompose::smileThemeMenu()
{
    QMenu *m = new QMenu(hub);

    for (const auto &f : QDir(WulforUtil::getInstance()->getEmoticonsPath())
                              .entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot)) {
        if (f.isEmpty())
            continue;

        QAction *act = m->addAction(f);
        act->setCheckable(true);

        if (f == WSGET(WS_APP_EMOTICON_THEME)) {
            act->setChecked(false);
            act->setChecked(true);
        }
    }

    QAction *a = m->exec(QCursor::pos());

    if (a && a->isChecked())
        WSSET(WS_APP_EMOTICON_THEME, a->text());

    m->deleteLater();
}

void HubChatCompose::clearPanel()
{
    QLayout *l = hub->frame_SMILES->layout();

    if (!l)
        return;

    QLayoutItem *item = nullptr;
    while ((item = l->takeAt(0))) {
        if (item->widget()) {
            l->removeWidget(item->widget());
            item->widget()->deleteLater();
        }
        delete item;
    }

    l->invalidate();
}

void HubChatCompose::rebuildPanel()
{
    if (!EmoticonFactory::getInstance())
        return;

    hub->frame_SMILES->setVisible(false);
    clearPanel();

    QSize sz;
    EmoticonFactory::getInstance()->fillLayout(hub->frame_SMILES->layout(), sz);

    for (const auto &l : hub->frame_SMILES->findChildren<EmoticonLabel *>())
        QObject::connect(l, SIGNAL(clicked()), hub, SLOT(slotSmileClicked()));
}
