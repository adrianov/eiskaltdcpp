/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "SideBar.h"
#include "ArenaWidgetManager.h"

void SideBarModel::insertWidget(ArenaWidget *awgt){
    if (items.contains(awgt) || !awgt)
        return;

    QModelIndex ind;

    switch (awgt->role()){
    case ArenaWidget::Hub:
    case ArenaWidget::PrivateMessage:
    case ArenaWidget::Search:
    case ArenaWidget::ShareBrowser:
    case ArenaWidget::CustomWidget:
    {
        SideBarItem *i = new SideBarItem(awgt, roots[awgt->role()]);
        roots[awgt->role()]->appendChild(i);

        items.insert(awgt, i);

        ind = index(i->row(), 0, index(roots[awgt->role()]->row(), 0, QModelIndex()));

        break;
    }
    default:
        roots[awgt->role()]->setWdget(awgt);
        
        SideBarItem *root  = roots[awgt->role()];
        
        ind = index(root->row(), 0, QModelIndex());
        
        if (root->getWidget() && root->getWidget()->toolButton())
            root->getWidget()->toolButton()->setChecked(true);

        break;
    }

    emit layoutChanged();
}

void SideBarModel::removeWidget(ArenaWidget *awgt){
    if (!items.contains(awgt) || !awgt)
        return;

    switch (awgt->role()){
    case ArenaWidget::Hub:
    case ArenaWidget::PrivateMessage:
    case ArenaWidget::Search:
    case ArenaWidget::ShareBrowser:
    case ArenaWidget::CustomWidget:
    {
        SideBarItem *root  = roots[awgt->role()];
        SideBarItem *child = items[awgt];
        const int row = child->row();
        const ArenaWidget::Role role = awgt->role();

        items.remove(awgt);
        redrawCache.remove(awgt);

        QModelIndex par_root = index(root->row(), 0, QModelIndex());

        beginRemoveRows(par_root, child->row(), child->row());
        {
            root->childItems.removeAt(root->childItems.indexOf(child));

            delete child;
        }
        endRemoveRows();

        // Prefer the next Search sibling over history when closing a Search.
        if (role == ArenaWidget::Search && root->childCount() > 0) {
            historyPurge(awgt);
            const int nextRow = qMin(row, root->childCount() - 1);
            if (ArenaWidget *next = root->child(nextRow)->getWidget())
                emit mapWidget(next);
            break;
        }

        if (!historyAtTop(awgt))
            historyPurge(awgt);

        historyPop();

        break;
    }
    default:
    {
        SideBarItem *root  = roots[awgt->role()];

        if (root->getWidget() && root->getWidget()->toolButton())
            root->getWidget()->toolButton()->setChecked(false);

        historyPop();
    }
    }
}

void SideBarModel::toggled ( ArenaWidget* awgt ) {
    if (!awgt)
        return;
    
    if (awgt->toolButton()){
        awgt->toolButton()->setChecked(false);
        awgt->toolButton()->setCheckable(false);
    }
    
    ArenaWidgetManager::getInstance()->activate(awgt);
}

bool SideBarModel::hasWidget(ArenaWidget *awgt) const{
    if (!awgt)
        return false;

    bool inRoot = false;

    for (const auto &root : roots){
        if (root->getWidget() == awgt && awgt->getWidget()->isVisible()){
            inRoot = true;

            break;
        }
    }

    return (items.contains(awgt) || inRoot);
}

void SideBarModel::mapped(ArenaWidget *awgt){
    if (!awgt)
        return;

    QModelIndex s;

    if (items.contains(awgt)){
        SideBarItem *root  = roots[awgt->role()];
        SideBarItem *child = items[awgt];

        QModelIndex par_root = index(root->row(), 0, QModelIndex());
        s = index(child->row(), 0, par_root);
    }
    else {
        SideBarItem *root  = roots[awgt->role()];

        s = index(root->row(), 0, QModelIndex());
    }

    historyPush(awgt);

    emit selectIndex(s);
}

void SideBarModel::updated ( ArenaWidget* awgt ) {
    if (!(awgt  && (awgt->state() & ArenaWidget::Singleton) && !historyStack.isEmpty()))
        return;
    
    if ((awgt->state() & ArenaWidget::Hidden) && (historyStack.last() == awgt))
        historyPop();
}

bool SideBarModel::isRootItem(const SideBarItem *item) const{
    for (const auto &root : roots){
        if (root == item)
            return true;
    }

    return false;
}

ArenaWidget::Role SideBarModel::rootItemRole(const SideBarItem *item) const{
    for (auto it = roots.begin(); it != roots.end(); ++it){
        if (it.value() == item)
            return it.key();
    }

    return ArenaWidget::CustomWidget;
}

