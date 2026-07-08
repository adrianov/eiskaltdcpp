/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferView.h"
#include "WulforUtil.h"

#include "dcpp/HashManager.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>

void TransferView::copyMenuSelection(const QList<TransferViewItem*> &items, int col){
    QString data;

    if (col <= (model->columnCount()-1)){
        for (const auto &i : items)
            data += i->data(col).toString() + "\n";
    }
    else {
        for (const auto &i : items){
            QFileInfo fi(i->target);
            QString tth_str = getTTHFromItem(i);

            if (tth_str.isEmpty()) {
                QString str = QDir::toNativeSeparators(fi.canonicalFilePath());
                const TTHValue *tth = HashManager::getInstance()->getFileTTHif(str.toStdString());

                if (tth)
                    tth_str = _q(tth->toBase32());
            }

            if (!tth_str.isEmpty())
                data += WulforUtil::getInstance()->makeMagnet(fi.fileName(), fi.size(), tth_str) + "\n";
        }
    }

    if (!data.isEmpty())
        qApp->clipboard()->setText(data, QClipboard::Clipboard);
}
