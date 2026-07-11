/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "HubFrame.h"
#include "AppTheme.h"
#include "WulforUtil.h"
#include "WulforSettings.h"

#include "dcpp/HashManager.h"

#include <QColor>
#include <QFileInfo>
#include <QRegExp>
#include <QUrl>

#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#endif

QString hubFrameMagnetTitle(const QString &link)
{
    QString magnet = link;

#if QT_VERSION >= 0x050000
    QUrlQuery u;
#else
    QUrl u;
#endif

    if (!magnet.contains("+")) {
#if QT_VERSION >= 0x050000
        u.setQuery(magnet.toUtf8());
#else
        u.setEncodedUrl(magnet.toUtf8());
#endif
    } else {
        QString encoded = magnet;
        encoded.replace("+", "%20");
#if QT_VERSION >= 0x050000
        u.setQuery(encoded.toUtf8());
#else
        u.setEncodedUrl(encoded.toUtf8());
#endif
    }

    if (u.hasQueryItem("kt")) {
        QString keywords = u.queryItemValue("kt");
        QString hub = u.hasQueryItem("xs") ? u.queryItemValue("xs") : QString();
        if (!(hub.startsWith("dchub://", Qt::CaseInsensitive) ||
              hub.startsWith("nmdcs://", Qt::CaseInsensitive) ||
              hub.startsWith("adc://", Qt::CaseInsensitive) ||
              hub.startsWith("adcs://", Qt::CaseInsensitive)) && !hub.isEmpty())
            hub.prepend("dchub://");

        if (keywords.isEmpty())
            keywords = HubFrame::tr("Invalid keywords");

#if QT_VERSION >= 0x050000
        return hub.isEmpty() ? keywords.toHtmlEscaped()
                             : keywords.toHtmlEscaped() + " (" + hub.toHtmlEscaped() + ")";
#else
        return hub.isEmpty() ? Qt::escape(keywords)
                             : Qt::escape(keywords) + " (" + Qt::escape(hub) + ")";
#endif
    }

    QString name, tth;
    int64_t size = 0;
    WulforUtil::splitMagnet(link, size, tth, name);

    if (link.contains("urn:btih:") || link.contains("urn:btmh:"))
        return QString("%1 (BitTorrent)").arg(name);
    if (size == 0 && tth.isEmpty())
        return QString("%1 (%2)").arg(name).arg(HubFrame::tr("search"));
    return QString("%1 (%2)").arg(name).arg(WulforUtil::formatBytes(size));
}

static bool parseBasicBBCode(const QString &tag, const QString &txt, QString &input, QString &output){
    if (tag.isEmpty())
        return false;

    const QString bbCode1 = QString("[%1]").arg(tag);
    const QString bbCode2 = QString("[/%1]").arg(tag);

    if (input.startsWith(bbCode1) && input.indexOf(bbCode2) >= bbCode1.length()){
        input.remove(0, bbCode1.length());
        const int c_len = input.indexOf(bbCode2);
        const QString chunk = HubFrame::LinkParser::parseForLinks(input.left(c_len), false);
        output += QString("<%1>").arg(txt) + chunk + QString("</%1>").arg(txt);
        input.remove(0, c_len + bbCode2.length());
        return true;
    }

    return false;
}

bool hubFrameTryBbCode(QString &input, QString &output)
{
    if (!WBGET("hubframe/use-bb-code", false))
        return false;

    if (parseBasicBBCode("b", "b", input, output) ||
        parseBasicBBCode("u", "u", input, output) ||
        parseBasicBBCode("i", "i", input, output) ||
        parseBasicBBCode("s", "s", input, output))
        return true;

    if (input.startsWith("[color=") && input.indexOf("[/color]") > 8){
        QRegExp exp("\\[color=(\\w+|#.{6,6})\\]((.*))\\[/color\\].*");
        QString chunk = input.left(input.indexOf("[/color]") + 8);
        if (exp.exactMatch(chunk) && exp.captureCount() == 3){
            QColor bbColor;
            bbColor.setNamedColor(exp.cap(1));
            output += "<font color=\"" + AppTheme::readableChatColor(bbColor).name() + "\">"
                    + HubFrame::LinkParser::parseForLinks(exp.cap(2), false) + "</font>";
            input.remove(0, chunk.length());
            return true;
        }
    } else if (input.startsWith("[url") && input.indexOf("[/url]") > 0){
        QRegExp exp("\\[url=*((.+[^\\]\\[]))*\\]((.+))\\[/url\\]");
        QString chunk = input.left(input.indexOf("[/url]") + 6);
        if (exp.exactMatch(chunk) && exp.captureCount() == 4){
            QString link = exp.cap(2);
            QString title = exp.cap(3);
            link = link.isEmpty() ? title : link;
            if (link.startsWith("="))
                link.remove(0, 1);
            if (!title.isEmpty()){
#if QT_VERSION >= 0x050000
                output += "<a href=\"" + link + "\" title=\"" + title.toHtmlEscaped() + "\">"
                        + title.toHtmlEscaped() + "</a>";
#else
                output += "<a href=\"" + link + "\" title=\"" + Qt::escape(title) + "\">"
                        + Qt::escape(title) + "</a>";
#endif
                input.remove(0, chunk.length());
                return true;
            }
        }
    } else if (input.startsWith("[code]") && input.indexOf("[/code]") > 0){
        input.remove(0, 6);
        const int c_len = input.indexOf("[/code]");
        output += "<table border=1 width=100%><tr><td align=\"left\">Code:</td></tr>"
                  "<tr><td align=\"left\"><pre style=\"white-space: pre;\"><tt>"
                + input.left(c_len) + "</tt></pre></td></tr></table>";
        input.remove(0, c_len + 7);
        return true;
    }

    return false;
}

void HubFrame::LinkParser::parseForMagnetAlias(QString &output){
    int pos = 0;
    QRegExp rx("(<magnet(?:\\s+show=([^>]+))?>(.+)</magnet>)");
    rx.setMinimal(true);
    while ((pos = output.indexOf(rx, pos)) >= 0) {
        QFileInfo fi(rx.cap(3));
        if (fi.isDir() || !fi.exists()) {
            pos++;
            continue;
        }
        QString name = fi.fileName();
        if (!rx.cap(2).isEmpty())
            name = rx.cap(2);

        const TTHValue *tth = HashManager::getInstance()->getFileTTHif(_tq(fi.absoluteFilePath()));
        if (tth) {
            QString urlStr = WulforUtil::getInstance()->makeMagnet(name, fi.size(), _q(tth->toBase32()));
            output.replace(pos, rx.cap(1).length(), urlStr);
        } else {
            output.replace(pos, rx.cap(1).length(), HubFrame::tr("not shared"));
        }
    }
}
