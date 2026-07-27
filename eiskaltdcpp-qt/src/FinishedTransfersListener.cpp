/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfers.h"

template <bool isUpload>
void FinishedTransfers<isUpload>::on(FinishedManagerListener::AddedFile, bool upload, const std::string &file, const FinishedFileItemPtr &item) noexcept
{
    if (isUpload != upload)
        return;

    VarMap params;
    bool show = false;
    {
        auto lock = FinishedManager::getInstance()->lock();
        show = showDownload(file, item);
        if (show)
            getParams(item, file, params);
    }

    if (!show) {
        if (!isUpload && isFileListPath(file)) {
            try {
                FinishedManager::getInstance()->remove(false, file);
            } catch (const std::exception&) {}
        }
        return;
    }

    persistFile(params);
    emit coreAddedFile(params);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::on(FinishedManagerListener::AddedUser, bool upload, const dcpp::HintedUser &user, const FinishedUserItemPtr &item) noexcept
{
    if (isUpload != upload)
        return;

    VarMap params;
    {
        auto lock = FinishedManager::getInstance()->lock();
        getParams(item, user, params);
    }
    persistUser(params);
    emit coreAddedUser(params);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::on(FinishedManagerListener::UpdatedFile, bool upload, const std::string &file, const FinishedFileItemPtr &item) noexcept
{
    if (isUpload != upload)
        return;

    VarMap params;
    {
        auto lock = FinishedManager::getInstance()->lock();
        if (!showDownload(file, item))
            return;
        getParams(item, file, params);
    }
    persistFile(params);
    emit coreUpdatedFile(params);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::on(FinishedManagerListener::RemovedFile, bool upload, const std::string &file) noexcept
{
    if (isUpload != upload)
        return;

    removeFileDB(_q(file));
    emit coreRemovedFile(_q(file));
}

template <bool isUpload>
void FinishedTransfers<isUpload>::on(FinishedManagerListener::UpdatedUser, bool upload, const dcpp::HintedUser &user) noexcept
{
    if (isUpload != upload)
        return;

    VarMap params;
    {
        auto lock = FinishedManager::getInstance()->lock();
        const FinishedManager::MapByUser &umap = FinishedManager::getInstance()->getMapByUser(isUpload);
        auto userit = umap.find(user);
        if (userit == umap.end())
            return;
        getParams(userit->second, user, params);
    }
    persistUser(params);
    emit coreUpdatedUser(params);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::on(FinishedManagerListener::RemovedUser, bool upload, const dcpp::HintedUser &user) noexcept
{
    if (isUpload == upload){
        emit coreRemovedUser(_q(user.user->getCID().toBase32()));
    }
}

template void FinishedTransfers<true>::on(FinishedManagerListener::AddedFile, bool, const std::string&, const FinishedFileItemPtr&) noexcept;
template void FinishedTransfers<false>::on(FinishedManagerListener::AddedFile, bool, const std::string&, const FinishedFileItemPtr&) noexcept;
template void FinishedTransfers<true>::on(FinishedManagerListener::AddedUser, bool, const dcpp::HintedUser&, const FinishedUserItemPtr&) noexcept;
template void FinishedTransfers<false>::on(FinishedManagerListener::AddedUser, bool, const dcpp::HintedUser&, const FinishedUserItemPtr&) noexcept;
template void FinishedTransfers<true>::on(FinishedManagerListener::UpdatedFile, bool, const std::string&, const FinishedFileItemPtr&) noexcept;
template void FinishedTransfers<false>::on(FinishedManagerListener::UpdatedFile, bool, const std::string&, const FinishedFileItemPtr&) noexcept;
template void FinishedTransfers<true>::on(FinishedManagerListener::RemovedFile, bool, const std::string&) noexcept;
template void FinishedTransfers<false>::on(FinishedManagerListener::RemovedFile, bool, const std::string&) noexcept;
template void FinishedTransfers<true>::on(FinishedManagerListener::UpdatedUser, bool, const dcpp::HintedUser&) noexcept;
template void FinishedTransfers<false>::on(FinishedManagerListener::UpdatedUser, bool, const dcpp::HintedUser&) noexcept;
template void FinishedTransfers<true>::on(FinishedManagerListener::RemovedUser, bool, const dcpp::HintedUser&) noexcept;
template void FinishedTransfers<false>::on(FinishedManagerListener::RemovedUser, bool, const dcpp::HintedUser&) noexcept;
