/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "PmSpamFilter.h"
#include "WulforUtil.h"

#include <QFile>
#include <QTextStream>

#include "dcpp/Util.h"

using namespace dcpp;

PmSpamFilter::PmSpamFilter(){
    load();
}

PmSpamFilter::~PmSpamFilter(){
    save();
}

QString PmSpamFilter::normalize(const QString &msg){
    QString t = msg;
    t.replace("\r\n", "\n");
    t.replace('\r', '\n');
    return t.trimmed();
}

bool PmSpamFilter::isSpam(const QString &msg) const {
    const QString key = normalize(msg);
    return !key.isEmpty() && texts.contains(key);
}

void PmSpamFilter::add(const QString &msg){
    const QString key = normalize(msg);
    if (key.isEmpty() || texts.contains(key))
        return;

    texts.insert(key);
    save();
}

void PmSpamFilter::addAll(const QStringList &msgs){
    bool changed = false;

    for (const QString &msg : msgs){
        const QString key = normalize(msg);
        if (key.isEmpty() || texts.contains(key))
            continue;

        texts.insert(key);
        changed = true;
    }

    if (changed)
        save();
}

void PmSpamFilter::load(){
    const QString path = _q(Util::getPath(Util::PATH_USER_CONFIG)) + "pmspam";
    QFile f(path);

    if (!(f.exists() && f.open(QIODevice::ReadOnly)))
        return;

    QTextStream stream(&f);

    while (!stream.atEnd()){
        const QByteArray raw = QByteArray::fromBase64(stream.readLine().toLatin1());
        const QString key = normalize(QString::fromUtf8(raw));

        if (!key.isEmpty())
            texts.insert(key);
    }
}

void PmSpamFilter::save() const {
    const QString path = _q(Util::getPath(Util::PATH_USER_CONFIG)) + "pmspam";
    QFile f(path);

    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    QTextStream stream(&f);

    for (const QString &msg : texts)
        stream << QString::fromLatin1(msg.toUtf8().toBase64()) << "\n";
}
