/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "FileBrowserModelSort.h"
#include "NaturalCompareQt.h"

namespace {

template <Qt::SortOrder order>
struct Compare {
    typedef bool (*AttrComp)(const FileBrowserItem * l, const FileBrowserItem * r);

    void static sort(unsigned column, QList<FileBrowserItem*>& items) {
        if (column > NUM_OF_COLUMNS-1)
            return;

        std::stable_sort(items.begin(), items.end(), attrs[column]);
    }

    private:
        template <int i>
        bool static AttrCmp(const FileBrowserItem * l, const FileBrowserItem * r) {
            if ((l->dir && !r->dir) || (!l->dir && r->dir))
                return (l->dir != nullptr);
            return Cmp(QString::localeAwareCompare(l->data(i).toString(), r->data(i).toString()), 0);
        }
        template <int i>
        bool static NaturalAttrCmp(const FileBrowserItem * l, const FileBrowserItem * r) {
            if ((l->dir && !r->dir) || (!l->dir && r->dir))
                return (l->dir != nullptr);
            return Cmp(compareNaturalQ(l->data(i).toString(), r->data(i).toString()), 0);
        }
        template <int column>
        bool static NumCmp(const FileBrowserItem * l, const FileBrowserItem * r) {
            if ((l->dir && !r->dir) || (!l->dir && r->dir))
                return (l->dir != nullptr);
            return Cmp(l->data(column).toULongLong(), r->data(column).toULongLong());
       }
        template <typename T>
        bool static Cmp(const T& l, const T& r);

        static AttrComp attrs[NUM_OF_COLUMNS];
};

template <Qt::SortOrder order>
typename Compare<order>::AttrComp Compare<order>::attrs[NUM_OF_COLUMNS] = {  NaturalAttrCmp<COLUMN_FILEBROWSER_NAME>,
                                                                NumCmp<COLUMN_FILEBROWSER_ESIZE>,
                                                                NumCmp<COLUMN_FILEBROWSER_ESIZE>,
                                                                AttrCmp<COLUMN_FILEBROWSER_TTH>,
                                                                NumCmp<COLUMN_FILEBROWSER_BR>,
                                                                AttrCmp<COLUMN_FILEBROWSER_WH>,
                                                                AttrCmp<COLUMN_FILEBROWSER_MVIDEO>,
                                                                AttrCmp<COLUMN_FILEBROWSER_MAUDIO>,
                                                                NumCmp<COLUMN_FILEBROWSER_HIT>,
                                                                AttrCmp<COLUMN_FILEBROWSER_TS>
                                                             };

template <> template <typename T>
bool inline Compare<Qt::AscendingOrder>::Cmp(const T& l, const T& r) {
    return l < r;
}

template <> template <typename T>
bool inline Compare<Qt::DescendingOrder>::Cmp(const T& l, const T& r) {
    return l > r;
}

void sortRecursive(int column, Qt::SortOrder order, FileBrowserItem *i){
    static Compare<Qt::AscendingOrder> acomp;
    static Compare<Qt::DescendingOrder> dcomp;

    if (column < 0 || !i || !i->childCount())
        return;

    if (order == Qt::AscendingOrder)
        acomp.sort(column, i->childItems);
    else if (order == Qt::DescendingOrder)
        dcomp.sort(column, i->childItems);

    for (const auto &ii : i->childItems)
        sortRecursive(column, order, ii);
}

} // namespace

void sortFileBrowserItems(int column, Qt::SortOrder order, FileBrowserItem *root) {
    sortRecursive(column, order, root);
}
