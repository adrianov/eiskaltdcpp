/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "HubFrame.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "EmoticonFactory.h"

#include <QUrl>

QString hubFrameMagnetTitle(const QString &link);
bool hubFrameTryBbCode(QString &input, QString &output);

QString HubFrame::LinkParser::parseForLinks(QString input, bool use_emot){
    if (input.isEmpty())
        return input;

    static const QList<QChar> unwise_chars = QList<QChar>() << '{' << '}' << '|' << '\\' << '^' << '[' << ']' << '`';
    static const QStringList link_types = QStringList() << "http://" << "https://" << "ftp://" << "www."
                                                  << "dchub://" << "nmdcs://" << "adc://" << "adcs://" << "magnet:";

    QString output;

    EmoticonMap emoticons;
    if (use_emot && WBGET(WB_APP_ENABLE_EMOTICON) && EmoticonFactory::getInstance())
        emoticons = EmoticonFactory::getInstance()->getEmoticons();

    const QString emo_theme = WSGET(WS_APP_EMOTICON_THEME);
    const bool force_emot = WBGET(WB_APP_FORCE_EMOTICONS);

    while (!input.isEmpty()){
        for (const QString &linktype : link_types){
            if (!input.startsWith(linktype))
                continue;

            int l_pos = linktype.length();
            while (l_pos < input.size()){
                const QChar ch = input.at(l_pos);
                if (ch.isSpace() || ch == '\n' || ch == '>' || ch == '<' || unwise_chars.contains(ch))
                    break;
                ++l_pos;
            }

            QString link = input.left(l_pos);
            QString toshow = link;

            if (linktype == "http://" || linktype == "https://" || linktype == "ftp://"){
                while (!QUrl(link).isValid() && !link.isEmpty()){
                    input.prepend(link.at(link.length() - 1));
                    link.chop(1);
                }
                toshow = QUrl::fromEncoded(link.toUtf8()).toString();
            } else if (linktype == "magnet:"){
                toshow = hubFrameMagnetTitle(link);
            }

            if (linktype == "www.")
                toshow.prepend("http://");

            if (linktype != "magnet:")
                output += QString("<a href=\"%1\" title=\"%1\" style=\"cursor: hand\">%1</a>").arg(toshow);
            else
                output += "<a href=\"" + link + "\" title=\"" + toshow + "\" style=\"cursor: hand\">" + toshow + "</a>";

            input.remove(0, l_pos);
            break;
        }

        if (input.isEmpty())
            break;

        bool smile_found = false;
        for (const QString &emo_text : emoticons.keys()){
            EmoticonObject *obj = emoticons[emo_text];
            if (!input.startsWith(emo_text) || !obj)
                continue;

            const auto appendSmile = [&](){
                output += QString("<img alt=\"%1\" title=\"%1\" align=\"center\" source=\"%2/emoticon%3\" />")
                              .arg(emo_text).arg(emo_theme).arg(obj->id);
                input.remove(0, emo_text.length());
                smile_found = true;
            };

            if (force_emot || input == emo_text){
                appendSmile();
                break;
            }

            if (!(output.endsWith(' ') || output.endsWith('\t') || output.isEmpty()))
                continue;

            const int emo_len = emo_text.length();
            const bool nextSpace = (input.length() == emo_len) ||
                                   (input.length() > emo_len &&
                                    (input.at(emo_len) == ' ' || input.at(emo_len) == '\t'));
            if (!nextSpace)
                continue;

            appendSmile();
            break;
        }

        if (smile_found)
            continue;

        if (hubFrameTryBbCode(input, output))
            continue;

        if (input.startsWith("<")){
            output += "&lt;";
            input.remove(0, 1);
            continue;
        }
        if (input.startsWith(">")){
            output += "&gt;";
            input.remove(0, 1);
            continue;
        }
        if (input.startsWith("&")){
            input.remove(0, 1);
            output += "&amp;";
            continue;
        }

        if (input.startsWith("[magnet=\"") && input.indexOf("[/magnet]") > 9){
            QString chunk = input.left(input.indexOf("[/magnet]"));
            do {
                if (chunk.isEmpty())
                    break;
                chunk.remove(0, 9);
                if (chunk.isEmpty() || chunk.indexOf("\"") <= 0)
                    break;
                QString magnet = chunk.left(chunk.indexOf("\"")).trimmed();
                if (magnet.isEmpty())
                    break;

                QString name, tth;
                int64_t size = 0;
                WulforUtil::splitMagnet(magnet, size, tth, name);
                chunk.remove(0, magnet.length());
                if (chunk.indexOf("]") < 1)
                    break;
                chunk.remove(0, chunk.indexOf("]") + 1);
                if (chunk.isEmpty())
                    break;

                const QString toshow = HubFrame::tr("%1 (%2)").arg(chunk).arg(WulforUtil::formatBytes(size));
                output += "<a href=\"" + magnet + "\" title=\"" + toshow + "\" style=\"cursor: hand\">" + toshow + "</a>";
                input.remove(0, input.indexOf("[/magnet]") + 10);
            } while (false);
        }

        if (input.isEmpty())
            break;

        output += input.at(0);
        input.remove(0, 1);
    }

    return output;
}
