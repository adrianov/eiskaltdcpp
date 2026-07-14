/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ShareBrowser.h"
#include "WulforUtil.h"
#include "FileBrowserModel.h"
#include "MainWindow.h"
#include "SearchLocalPath.h"

#include "dcpp/ClientManager.h"
#include "dcpp/Exception.h"
#include "dcpp/QueueManager.h"
#include "dcpp/SettingsManager.h"

#include <QAbstractItemView>
#include <vector>

using namespace dcpp;

namespace {

struct DlJob {
    string target;
    int64_t size;
    TTHValue tth;
};

void collectJobs(DirectoryListing::Directory *dir, DirectoryListing::Directory *root,
                 const string &aTarget, std::vector<DlJob> &out)
{
    const string target = (dir == root) ? aTarget : aTarget + dir->getName() + PATH_SEPARATOR;
    for (auto &sub : dir->directories)
        collectJobs(sub, root, target, out);
    for (auto *file : dir->files)
        out.push_back({target + file->getName(), file->getSize(), file->getTTH()});
}

void queueJobs(const HintedUser &user, std::vector<DlJob> jobs)
{
    if (jobs.empty())
        return;

    AsyncRunner *runner = new AsyncRunner(nullptr);
    runner->setRunFunction([user, jobs = std::move(jobs)]() {
        for (const DlJob &job : jobs) {
            try {
                QueueManager::getInstance()->add(job.target, job.size, job.tth, user, 0);
            } catch (const Exception &) {
            }
        }
    });
    QObject::connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()), Qt::QueuedConnection);
    runner->start();
}

} // namespace

void ShareBrowser::goDown(QTreeView *view){
    if (view != treeView_RPANE)
        return;

    QItemSelectionModel *selection_model = view->selectionModel();
    QModelIndexList selected  = selection_model->selectedRows(0);

    if (selected.size() > 1 || selected.empty())
        return;

    const QModelIndex index = selected.at(0);
    FileBrowserItem *item = nullptr;

    if (view->model() == proxy)
        item = static_cast<FileBrowserItem*>(proxy->mapToSource(index).internalPointer());
    else
        item = static_cast<FileBrowserItem*>(index.internalPointer());

    if (!item || item->file)
        return;

    slotRightPaneClicked(index);

    treeView_RPANE->setFocus();
}

void ShareBrowser::goUp(QTreeView *view){
    if (view != treeView_RPANE)
        return;

    QStringList paths = lineEdit_PATH->text().split("\\", WULFOR_SKIP_EMPTY);

    if (paths.empty())//is it possible?
        return;
    else
        paths.removeLast();

    FileBrowserItem *tree_item = tree_model->createRootForPath(paths.join("\\"));
    QModelIndex tree_index = tree_model->createIndexForItem(tree_item);

    treeView_LPANE->selectionModel()->setCurrentIndex(tree_index,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    treeView_LPANE->scrollTo(tree_index, QAbstractItemView::PositionAtCenter);

    treeView_RPANE->setFocus();
}

void ShareBrowser::download(dcpp::DirectoryListing::Directory *dir, const QString &target){
    if (!dir || target.isEmpty())
        return;

    // Snapshot targets on the UI thread so the worker never touches listing (or a closed tab).
    std::vector<DlJob> jobs;
    collectJobs(dir, listing.getRoot(), target.toStdString(), jobs);
    queueJobs(listing.getUser(), std::move(jobs));
}

void ShareBrowser::download(dcpp::DirectoryListing::File *file, const QString &target){
    if (!file || target.isEmpty())
        return;

    try {
        listing.download(file, (target + _q(file->getName())).toStdString(), false, false);
    } catch (const Exception &e) {
        MainWindow::getInstance()->setStatusMessage(_q(e.getError()));
    }
}

void ShareBrowser::slotRightPaneClicked(const QModelIndex &index){
    if (!index.isValid())
        return;

    FileBrowserItem *item = nullptr;

    if (treeView_RPANE->model() == proxy)
        item = static_cast<FileBrowserItem*>(proxy->mapToSource(index).internalPointer());
    else
        item = static_cast<FileBrowserItem*>(index.internalPointer());

    if (!item)
        return;

    if (item->file){
        if (user == ClientManager::getInstance()->getMe()) {
            for (const auto &path : listing.getLocalPaths(item->file))
                SearchLocalPath::openFile(_q(path));
        } else {
            download(item->file, _q(SETTING(DOWNLOAD_DIRECTORY)));
        }

        return;
    }

    QString parent_path = lineEdit_PATH->text() + "\\" + item->data(COLUMN_FILEBROWSER_NAME).toString();
    QModelIndex parent_index = tree_model->createIndexForItem(tree_model->createRootForPath(parent_path));

    if (!parent_index.isValid())
        return;

    if (tree_model->canFetchMore(parent_index))
        tree_model->fetchMore(parent_index);

    treeView_LPANE->selectionModel()->setCurrentIndex(parent_index,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    treeView_LPANE->scrollTo(parent_index, QAbstractItemView::PositionAtCenter);
}
