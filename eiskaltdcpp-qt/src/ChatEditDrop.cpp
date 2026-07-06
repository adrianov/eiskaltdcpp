/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ChatEdit.h"
#include "WulforUtil.h"

#include "dcpp/HashManager.h"

#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

void ChatEdit::dragMoveEvent(QDragMoveEvent *event) {
    event->accept();
}

void ChatEdit::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls() || e->mimeData()->hasText()) {
        e->acceptProposedAction();
    } else {
        e->ignore();
    }
}

void ChatEdit::dropEvent(QDropEvent *e)
{
    if (e->mimeData()->hasUrls()) {

        e->setDropAction(Qt::IgnoreAction);

        QStringList fileNames;
        for (const auto url : e->mimeData()->urls()) {
            QString urlStr = url.toString();
            if (url.scheme().toLower() == "file") {
                QFileInfo fi( url.toLocalFile() );
                QString str = QDir::toNativeSeparators( fi.absoluteFilePath() );

                if ( fi.exists() && fi.isFile() && !str.isEmpty() ) {
                    const TTHValue *tth = HashManager::getInstance()->getFileTTHif(str.toStdString());
                    if ( !tth ) {
                        str = QDir::toNativeSeparators( fi.canonicalFilePath() ); // try to follow symlinks
                        tth = HashManager::getInstance()->getFileTTHif(str.toStdString());
                    }
                    if (tth)
                        urlStr = WulforUtil::getInstance()->makeMagnet(fi.fileName(), fi.size(), _q(tth->toBase32()));
                }
            };

            if (!urlStr.isEmpty())
                fileNames << urlStr;
        }

        if (!fileNames.isEmpty()) {

            QString dropText = (fileNames.count() == 1) ? fileNames.last() : "\n" + fileNames.join("\n");

            QMimeData mime;
            mime.setText(dropText);
            QDropEvent drop(e->pos(), Qt::CopyAction, &mime, e->mouseButtons(),
                            e->keyboardModifiers(), e->type());

            QTextEdit::dropEvent(&drop);
            return;
        }
    }
    QTextEdit::dropEvent(e);
}
