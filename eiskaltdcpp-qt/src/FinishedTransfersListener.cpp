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

    if (!showDownload(file, item)) {
        if (!isUpload && isFileListPath(file)) {
            try {
                FinishedManager::getInstance()->remove(false, file);
            } catch (const std::exception&) {}
        }
        return;
    }

    VarMap params;

    getParams(item, file, params);

    emit coreAddedFile(params);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::on(FinishedManagerListener::AddedUser, bool upload, const dcpp::HintedUser &user, const FinishedUserItemPtr &item) noexcept
{
    if (isUpload == upload){
        VarMap params;

        getParams(item, user, params);

        emit coreAddedUser(params);
    }
}

template <bool isUpload>
void FinishedTransfers<isUpload>::on(FinishedManagerListener::UpdatedFile, bool upload, const std::string &file, const FinishedFileItemPtr &item) noexcept
{
    if (isUpload == upload && showDownload(file, item)){
        VarMap params;

        getParams(item, file, params);

        emit coreUpdatedFile(params);
    }
}

template <bool isUpload>
void FinishedTransfers<isUpload>::on(FinishedManagerListener::RemovedFile, bool upload, const std::string &file) noexcept
{
    if (isUpload == upload){
        removeFileFromDB(_q(file));
        emit coreRemovedFile(_q(file));
    }
}

template <bool isUpload>
void FinishedTransfers<isUpload>::on(FinishedManagerListener::UpdatedUser, bool upload, const dcpp::HintedUser &user) noexcept
{
    if (isUpload == upload){
        const FinishedManager::MapByUser &umap = FinishedManager::getInstance()->getMapByUser(isUpload);
        auto userit = umap.find(user);
        if (userit == umap.end())
            return;

        const FinishedUserItemPtr &item = userit->second;

        VarMap params;

        getParams(item, user, params);

        emit coreUpdatedUser(params);
    }
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
