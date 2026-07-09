/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforUtil.h"

#include "dcpp/FavoriteManager.h"
#include "dcpp/UserCommand.h"

#include <QMenu>
#include <QTreeView>
#include <QHeaderView>
#include <QAction>
#include <QAbstractItemModel>

using namespace dcpp;

void WulforUtil::headerMenu(QTreeView *tree){
    if (!tree || !tree->model() || !tree->header())
        return;

    QMenu * mcols = new QMenu(nullptr);
    QAbstractItemModel *model = tree->model();
    QAction * column;

    int count = 0;
    for (int i = 0; i < model->columnCount(); ++i)
        count += tree->header()->isSectionHidden(tree->header()->logicalIndex(i))? 0 : 1;

    bool allowDisable = count > 1;
    int index;

    for (int i = 0; i < model->columnCount(); ++i) {
        index = tree->header()->logicalIndex(i);
        column = mcols->addAction(model->headerData(index, Qt::Horizontal).toString());
        column->setCheckable(true);

        bool checked = !tree->header()->isSectionHidden(index);

        column->setChecked(checked);
        column->setData(index);

        if (checked && !allowDisable)
            column->setEnabled(false);
    }

    QAction * chosen = mcols->exec(QCursor::pos());

    if (chosen) {
        index = chosen->data().toInt();

        if (tree->header()->isSectionHidden(index)) {
            tree->header()->showSection(index);
        } else {
            tree->header()->hideSection(index);
        }
    }

    delete mcols;
}

QMenu *WulforUtil::buildUserCmdMenu(const QList<QString> &hub_list, int ctx, QWidget* parent) {
    dcpp::StringList hubs;
    for (const auto &hub : hub_list)
        hubs.push_back(_tq(hub));

    return buildUserCmdMenu(hubs, ctx, parent);
}

QMenu *WulforUtil::buildUserCmdMenu(const std::string& hub_url, int ctx, QWidget* parent) {
    return buildUserCmdMenu(StringList(1, hub_url), ctx, parent);
}

QMenu *WulforUtil::buildUserCmdMenu(const StringList& hub_list, int ctx, QWidget* parent) {
    UserCommand::List userCommands = FavoriteManager::getInstance()->getUserCommands(ctx, hub_list);

    if (userCommands.empty())
        return nullptr;

    QMenu *ucMenu = new QMenu(tr("User commands"), parent);

    QMenu *menuPtr = ucMenu;
    for (size_t n = 0; n < userCommands.size(); ++n) {
        UserCommand *uc = &userCommands[n];
        if (uc->getType() == UserCommand::TYPE_SEPARATOR) {
            // Avoid double separators...
            if (!menuPtr->actions().isEmpty() &&
                !menuPtr->actions().last()->isSeparator())
            {
                menuPtr->addSeparator();
            }
        } else if (uc->isRaw() || uc->isChat()) {
            menuPtr = ucMenu;
            auto _begin = uc->getDisplayName().begin();
            auto _end = uc->getDisplayName().end();
            for(; _begin != _end; ++_begin) {
                const QString name = _q(*_begin);
                if (_begin + 1 == _end) {
                    menuPtr->addAction(name)->setData(uc->getId());
                } else {
                    bool found = false;
                    QListIterator<QAction*> iter(menuPtr->actions());
                    while(iter.hasNext()) {
                        QAction *item = iter.next();
                        if (item->menu() && item->text() == name) {
                            found = true;
                            menuPtr = item->menu();
                            break;
                        }
                    }

                    if (!found) {
                        menuPtr = menuPtr->addMenu(name);
                    }
                }
            }
        }
    }
    return ucMenu;
}

