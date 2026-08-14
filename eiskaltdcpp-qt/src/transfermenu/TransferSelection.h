/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "TransferView.h"

#include <QList>
#include <QStringList>

class QTreeView;

/** Current Transfers-list selection: resolved paths and context-menu actions. */
class TransferSelection
{
public:
    TransferSelection(TransferView &view, QTreeView *tree);

    bool isEmpty() const { return items.isEmpty(); }
    bool canOpen() const { return !paths.isEmpty(); }
    bool canConvert() const;
    bool canRemove() const;

    void activateFiles() const;
    void run(TransferView::Menu::Action act, int copyColumn) const;

private:
    bool runFile(TransferView::Menu::Action act, int copyColumn) const;
    bool runPeer(TransferView::Menu::Action act) const;
    void runQueue(TransferView::Menu::Action act) const;
    void copyNames() const;
    void searchAlternates() const;
    void sendPM() const;

    TransferView &view;
    QList<TransferViewItem*> items;
    QStringList paths;
};
