/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfersModelSort.h"
#include "NaturalCompareQt.h"

#include <algorithm>

namespace FinishedTransfersModelSort {
namespace {

template <Qt::SortOrder order>
struct FileCompare {
    static void sort(int col, QList<FinishedTransfersItem*> &items) {
        std::stable_sort(items.begin(), items.end(), getAttrComp(col));
    }

    private:
        typedef bool (*AttrComp)(const FinishedTransfersItem *l, const FinishedTransfersItem *r);
        static AttrComp getAttrComp(int column) {
            switch (column){
                case COLUMN_FINISHED_NAME:
                    return NaturalAttrCmp<COLUMN_FINISHED_NAME>;
                case COLUMN_FINISHED_PATH:
                    return NaturalAttrCmp<COLUMN_FINISHED_PATH>;
                case COLUMN_FINISHED_TIME:
                    return AttrCmp<COLUMN_FINISHED_TIME>;
                case COLUMN_FINISHED_USER:
                    return AttrCmp<COLUMN_FINISHED_USER>;
                case COLUMN_FINISHED_TR:
                    return NumCmp<COLUMN_FINISHED_TR>;
                case COLUMN_FINISHED_SPEED:
                    return NumCmp<COLUMN_FINISHED_SPEED>;
                case COLUMN_FINISHED_CRC32:
                    return NumCmp<COLUMN_FINISHED_CRC32>;
                case COLUMN_FINISHED_TARGET:
                    return NaturalAttrCmp<COLUMN_FINISHED_TARGET>;
                case COLUMN_FINISHED_FULL:
                    return NumCmp<COLUMN_FINISHED_FULL>;
                default:
                    return NumCmp<COLUMN_FINISHED_ELAPS>;
            }
        }
        template <int i>
        static bool AttrCmp(const FinishedTransfersItem *l, const FinishedTransfersItem *r) {
            return Cmp(QString::localeAwareCompare(l->data(i).toString(), r->data(i).toString()), 0);
        }
        template <int i>
        static bool NaturalAttrCmp(const FinishedTransfersItem *l, const FinishedTransfersItem *r) {
            return Cmp(compareNaturalQ(l->data(i).toString(), r->data(i).toString()), 0);
        }
        template <int i>
        static bool NumCmp(const FinishedTransfersItem *l, const FinishedTransfersItem *r) {
            return Cmp(l->data(i).toULongLong(), r->data(i).toULongLong());
        }
        template <typename T>
        static bool Cmp(const T& l, const T& r);
};

template <> template <typename T>
bool inline FileCompare<Qt::AscendingOrder>::Cmp(const T& l, const T& r) {
    return l < r;
}

template <> template <typename T>
bool inline FileCompare<Qt::DescendingOrder>::Cmp(const T& l, const T& r) {
    return l > r;
}

template <Qt::SortOrder order>
struct UserCompare {
    static void sort(int col, QList<FinishedTransfersItem*> &items) {
        std::stable_sort(items.begin(), items.end(), getAttrComp(col));
    }

    private:
        typedef bool (*AttrComp)(const FinishedTransfersItem *l, const FinishedTransfersItem *r);
        static AttrComp getAttrComp(int column) {
            switch (column){
                case COLUMN_FINISHED_NAME:
                    return AttrCmp<COLUMN_FINISHED_NAME>;
                case COLUMN_FINISHED_PATH:
                    return AttrCmp<COLUMN_FINISHED_PATH>;
                case COLUMN_FINISHED_TIME:
                    return AttrCmp<COLUMN_FINISHED_TIME>;
                case COLUMN_FINISHED_USER:
                    return NumCmp<COLUMN_FINISHED_USER>;
                case COLUMN_FINISHED_TR:
                    return NumCmp<COLUMN_FINISHED_TR>;
                case COLUMN_FINISHED_FULL:
                    return NumCmp<COLUMN_FINISHED_FULL>;
                default:
                    return AttrCmp<COLUMN_FINISHED_ELAPS>;
            }
        }
        template <int i>
        static bool AttrCmp(const FinishedTransfersItem *l, const FinishedTransfersItem *r) {
            return Cmp(QString::localeAwareCompare(l->data(i).toString(), r->data(i).toString()), 0);
        }
        template <int i>
        static bool NumCmp(const FinishedTransfersItem *l, const FinishedTransfersItem *r) {
            return Cmp(l->data(i).toULongLong(), r->data(i).toULongLong());
        }
        template <typename T>
        static bool Cmp(const T& l, const T& r);
};

template <> template <typename T>
bool inline UserCompare<Qt::AscendingOrder>::Cmp(const T& l, const T& r) {
    return l < r;
}

template <> template <typename T>
bool inline UserCompare<Qt::DescendingOrder>::Cmp(const T& l, const T& r) {
    return l > r;
}

} // namespace

void sortFiles(int column, Qt::SortOrder order, QList<FinishedTransfersItem*> &items)
{
    if (order == Qt::AscendingOrder)
        FileCompare<Qt::AscendingOrder>().sort(column, items);
    else if (order == Qt::DescendingOrder)
        FileCompare<Qt::DescendingOrder>().sort(column, items);
}

void sortUsers(int column, Qt::SortOrder order, QList<FinishedTransfersItem*> &items)
{
    if (order == Qt::AscendingOrder)
        UserCompare<Qt::AscendingOrder>().sort(column, items);
    else if (order == Qt::DescendingOrder)
        UserCompare<Qt::DescendingOrder>().sort(column, items);
}

} // namespace FinishedTransfersModelSort
