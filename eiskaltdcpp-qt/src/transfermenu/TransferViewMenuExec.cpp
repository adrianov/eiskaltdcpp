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

#include "TransferView.h"
#include "transfermenu/TransferSelection.h"

void TransferView::slotDoubleClicked(const QModelIndex &)
{
    TransferSelection(*this, treeView_TRANSFERS).activateFiles();
}

void TransferView::slotContextMenu(const QPoint &)
{
    const TransferSelection sel(*this, treeView_TRANSFERS);
    if (sel.isEmpty())
        return;

    Menu m(model->getShowTranferedFilesOnlyState(), sel.canOpen(),
           sel.canRemove(), sel.canConvert());
    sel.run(m.exec(), m.copyColumn());
}
