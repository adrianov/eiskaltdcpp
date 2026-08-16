/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "EmoticonFactory.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include <QDir>
#include <QFile>
#include <QtDebug>
#include <QSizePolicy>

#include <math.h>

static const QString EmoticonSectionName = "emoticons-map";
static const QString EmoticonSubsectionName = "emoticon";
static const QString EmoticonTextSectionName = "name";

EmoticonFactory::EmoticonFactory() :
    QObject(nullptr)
{
}

EmoticonFactory::~EmoticonFactory(){
    clear();
}

void EmoticonFactory::load(){
    const QString emoTheme = WSGET(WS_APP_EMOTICON_THEME, "default");

    if (emoTheme.isEmpty())
        return;
    if (currentTheme == emoTheme && !map.isEmpty())
        return;

    if (!QDir(WulforUtil::getInstance()->getEmoticonsPath() + emoTheme).exists())
        return;

    const QString xmlFile = WulforUtil::getInstance()->getEmoticonsPath() + emoTheme + ".xml";

    if (!QFile::exists(xmlFile))
        return;

    QFile f(xmlFile);

    if (!f.open(QIODevice::ReadOnly))
        return;

    QDomDocument dom;
    const auto parsed = dom.setContent(&f);
    if (!parsed){
        qDebug() << parsed.errorLine << ":" << parsed.errorColumn << " " << parsed.errorMessage;
        f.close();
        return;
    }

    f.close();

    clear();
    createEmoticonMap(dom);
    currentTheme = emoTheme;

    for (const auto &d : docs)
        addEmoticons(d);
}

void EmoticonFactory::addEmoticons(QTextDocument *to){
    if (list.isEmpty() || !to)
        return;

    QString emoTheme = WSGET(WS_APP_EMOTICON_THEME);

    for (const auto &i : list){
        // Physical pixels via HQ scale (20 logical → 40/60/80 at @2/@3/@4), but
        // dpr=1.0 for QTextDocument ImageResource. HTML width/height stay 20;
        // text engine scales physical->logical so Retina stays sharp.
        const int logical = emoticonLogicalSide();
        QPixmap px = WulforUtil::scalePixmap(i->pixmap, logical);
        px.setDevicePixelRatio(1.0);
        to->addResource(QTextDocument::ImageResource,
                        QUrl(emoTheme + "/emoticon" + QString().setNum(i->id)),
                        px);
    }

    if (!docs.contains(to)){
        connect(to, SIGNAL(destroyed()), this, SLOT(slotDocDeleted()));

        docs << to;
    }
}

void EmoticonFactory::fillLayout(QLayout *l, QSize &recommendedSize){
    if (!l)
        return;

    int w = 0, h = 0, total = list.size();

    if (!total){
        recommendedSize = QSize(50, 50);
        return;
    }

    for (const auto &i : list){
        EmoticonLabel *lbl = new EmoticonLabel();

        const int logical = emoticonLogicalSide();
        const QPixmap px = WulforUtil::scalePixmap(i->pixmap, logical);
        const QSize cell = QSize(logical, logical) + QSize(2, 2);
        lbl->setContentsMargins(1, 1, 1, 1);
        lbl->setPixmap(px);
        // resize() is ignored by FlowLayout; pin logical cell size explicitly.
        lbl->setFixedSize(cell);
        lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        lbl->setToolTip(map.keys(i).first());

        w += cell.width();
        h  = cell.height();

        l->addWidget(lbl);
    }

    int square = w*h;
    int dim = static_cast<int>(sqrt(square));
    int extra = (dim/total);//for margins

    //10 extra pixels
    recommendedSize.setHeight(dim+extra+10);
    recommendedSize.setWidth(dim+extra+10);
}

void EmoticonFactory::clear(){
    qDeleteAll(list);

    map.clear();
    list.clear();
}

void EmoticonFactory::createEmoticonMap(const QDomNode &root){
    if (root.isNull())
        return;

    QDomNode r = findSectionByName(root, EmoticonSectionName);

    if (r.isNull())
        return;

    DomNodeList emoNodes;
    getSubSectionsByName(r, emoNodes, EmoticonSubsectionName);

    if (emoNodes.isEmpty())
        return;

    clear();

    for (const auto &node : emoNodes){
        QString emoTheme = WSGET(WS_APP_EMOTICON_THEME);

        EmoticonObject *emot = new EmoticonObject();
        QDomElement el = node.toElement();

        emot->fileName  = el.attribute("file").toUtf8();
        emot->id        = list.size();
        emot->pixmap    = QPixmap();
        emot->pixmap.load(WulforUtil::getInstance()->getEmoticonsPath() +
                          emoTheme + QDir::separator() + emot->fileName);

        DomNodeList emoTexts;
        getSubSectionsByName(node, emoTexts, EmoticonTextSectionName);

        if (emoTexts.isEmpty()){
            delete emot;

            continue;
        }

        int registered = 0;
        for (const auto &textNode : emoTexts){
            QDomElement textEl = textNode.toElement();

            if (textEl.isNull())
                continue;

            QString text = textEl.attribute("text").toUtf8();

            if (text.isEmpty() || map.contains(text))
                continue;

            registered++;

            map.insert(text, emot);
        }

        if (registered > 0)
            list.push_back(emot);
        else
            delete emot;
    }
}

QDomNode EmoticonFactory::findSectionByName(const QDomNode &node, const QString &name){
    QDomNode domNode = node.firstChild();

    while (!domNode.isNull()){
        if (domNode.isElement()){
            QDomElement domElement = domNode.toElement();

            if (!domElement.isNull() && domElement.tagName().toLower() == name.toLower())
                return domNode;
        }

        domNode = domNode.nextSibling();
    }

    return QDomNode();
}

void EmoticonFactory::getSubSectionsByName(const QDomNode &node, EmoticonFactory::DomNodeList &list, const QString &name){
    Q_UNUSED(name)

    QDomNode domNode = node.firstChild();

    while (!domNode.isNull()){
        list << domNode;

        domNode = domNode.nextSibling();
    }
}

void EmoticonFactory::slotDocDeleted(){
    QTextDocument *doc = reinterpret_cast<QTextDocument*>(sender());

    if (docs.contains(doc))
        docs.removeAt(docs.indexOf(doc));
}
