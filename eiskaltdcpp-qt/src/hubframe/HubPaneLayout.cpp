/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * Hub chat / user-list pane geometry. QTreeView size hints follow
 * header->length(); column autosize must not let that hint shove the list
 * over the chat inside the splitter.
 */

#include "hubframe/HubPaneLayout.h"

#include "WulforSettings.h"

#include <QAbstractButton>
#include <QColor>
#include <QComboBox>
#include <QEvent>
#include <QPalette>
#include <QSizePolicy>
#include <QSplitter>
#include <QTextEdit>
#include <QTreeView>
#include <QWidget>

namespace {
constexpr int kMinPane = 120;
}

HubPaneLayout::HubPaneLayout(QObject *host)
    : QObject(host)
{
}

void HubPaneLayout::bind(QSplitter *split, QTreeView *users, QTextEdit *chat)
{
    split_ = split;
    users_ = users;
    chat_ = chat;
    if (!split_ || !users_ || !chat_)
        return;

    split_->setHandleWidth(4);
    split_->setChildrenCollapsible(false);
    split_->setStretchFactor(0, 1);
    split_->setStretchFactor(1, 0);
    loosenListSize();
    split_->installEventFilter(this);
}

void HubPaneLayout::loosenListSize()
{
    if (QWidget *chatPane = split_->widget(0))
        chatPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (QWidget *userPane = split_->widget(1))
        userPane->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    users_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
}

void HubPaneLayout::restore()
{
    if (done_ || !split_)
        return;

    const int total = split_->width();
    if (total < kMinPane * 2)
        return; // Resize filter retries once the arena finishes layout.

    int chat = WIGET(WI_CHAT_WIDTH);
    int ulist = WIGET(WI_CHAT_USERLIST_WIDTH);
    if (chat < kMinPane || ulist < kMinPane)
        chat = total * 3 / 5;
    else
        chat = qBound(kMinPane, chat * total / (chat + ulist), total - kMinPane);

    split_->setSizes(QList<int>() << chat << (total - chat));
    done_ = true;
    split_->removeEventFilter(this);
}

void HubPaneLayout::save() const
{
    if (!split_)
        return;
    const QList<int> panes = split_->sizes();
    if (panes.size() < 2 || panes.at(0) <= 0 || panes.at(1) <= 0)
        return;
    WISET(WI_CHAT_WIDTH, panes.at(0));
    WISET(WI_CHAT_USERLIST_WIDTH, panes.at(1));
}

void HubPaneLayout::fillChat(const QColor &base)
{
    if (!chat_)
        return;

    QPalette p = chat_->palette();
    p.setColor(QPalette::Base, base);
    chat_->setPalette(p);
    chat_->setAutoFillBackground(true);
    // macOS styles often ignore QTextEdit Base; paint the viewport explicitly.
    if (QWidget *vp = chat_->viewport()) {
        vp->setPalette(p);
        vp->setAutoFillBackground(true);
        const QString css = QStringLiteral("background-color: %1;").arg(base.name(QColor::HexRgb));
        if (vp->styleSheet() != css)
            vp->setStyleSheet(css);
    }
}

void HubPaneLayout::alignFields(QWidget *filter, QComboBox *columns,
                                QWidget *input, QAbstractButton *smile)
{
    if (!filter || !columns || !input || !smile)
        return;
    const int h = qMax(filter->sizeHint().height(), columns->sizeHint().height());
    filter->setFixedHeight(h);
    columns->setFixedHeight(h);
    input->setMinimumHeight(h);
    smile->setFixedSize(h, h);
}

bool HubPaneLayout::eventFilter(QObject *obj, QEvent *ev)
{
    if (!done_ && obj == split_ && ev->type() == QEvent::Resize)
        restore();
    return QObject::eventFilter(obj, ev);
}
