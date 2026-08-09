/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * Keeps hub chat and user-list panes side by side: splitter sizes, list
 * size-hint isolation, and an opaque chat surface so the list cannot cover chat.
 */

#pragma once

#include <QObject>
#include <QPointer>

class QAbstractButton;
class QColor;
class QComboBox;
class QEvent;
class QSplitter;
class QTextEdit;
class QTreeView;
class QWidget;

class HubPaneLayout : public QObject
{
public:
    explicit HubPaneLayout(QObject *host);

    void bind(QSplitter *split, QTreeView *users, QTextEdit *chat);
    void restore();
    void save() const;
    void fillChat(const QColor &base);
    void alignFields(QWidget *filter, QComboBox *columns,
                     QWidget *input, QAbstractButton *smile);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    void loosenListSize();

    QPointer<QSplitter> split_;
    QPointer<QTreeView> users_;
    QPointer<QTextEdit> chat_;
    bool done_ = false;
};
