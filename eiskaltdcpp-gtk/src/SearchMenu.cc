/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Context menu build and download dispatch for Search results.
 */

#include "search.hh"

#include <dcpp/FavoriteManager.h>
#include <dcpp/QueueManager.h>
#include "UserCommandMenu.hh"
#include "wulformanager.hh"
#include "WulforUtil.hh"

using namespace std;
using namespace dcpp;

void Search::popupMenu_gui()
{
    GtkTreeIter iter;
    GtkTreePath *path;
    GList *list = gtk_tree_selection_get_selected_rows(selection, NULL);
    guint count = g_list_length(list);

    if (count == 1)
    {
        path = (GtkTreePath*)list->data; // This will be freed later

        // If it is a parent effectively more than one row is selected
        if (gtk_tree_model_get_iter(sortedFilterModel, &iter, path) &&
                gtk_tree_model_iter_has_child(sortedFilterModel, &iter))
        {
            gtk_widget_set_sensitive(getWidget("searchByTTHItem"), false);
        }
        else
        {
            gtk_widget_set_sensitive(getWidget("searchByTTHItem"), true);
        }
    }
    else if (count > 1)
    {
        gtk_widget_set_sensitive(getWidget("searchByTTHItem"), false);
    }

    GtkWidget *menuItem;
    string tth;
    bool firstTTH;
    bool hasTTH;

    // Clean menus
    gtk_container_foreach(GTK_CONTAINER(getWidget("downloadMenu")), (GtkCallback)gtk_widget_destroy, NULL);
    gtk_container_foreach(GTK_CONTAINER(getWidget("downloadDirMenu")), (GtkCallback)gtk_widget_destroy, NULL);
    userCommandMenu->cleanMenu_gui();

    // Build "Download to..." submenu

    // Add favorite download directories
    StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
    if (!spl.empty())
    {
        for (auto& i : spl)
        {
            menuItem = gtk_menu_item_new_with_label(i.second.c_str());
            g_object_set_data_full(G_OBJECT(menuItem), "fav", g_strdup(i.first.c_str()), g_free);
            g_signal_connect(menuItem, "activate", G_CALLBACK(onDownloadFavoriteClicked_gui), (gpointer)this);
            gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadMenu")), menuItem);
        }
        menuItem = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadMenu")), menuItem);
    }

    // Add Browse item
    menuItem = gtk_menu_item_new_with_label(_("Browse..."));
    g_signal_connect(menuItem, "activate", G_CALLBACK(onDownloadToClicked_gui), (gpointer)this);
    gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadMenu")), menuItem);

    // Add search results with the same TTH to menu
    firstTTH = true;
    hasTTH = false;

    for (GList *i = list; i; i = i->next)
    {
        path = (GtkTreePath *)i->data;
        if (gtk_tree_model_get_iter(sortedFilterModel, &iter, path))
        {
            userCommandMenu->addHub(resultView.getString(&iter, "Hub URL"));
            userCommandMenu->addFile(resultView.getString(&iter, "CID"),
                                     resultView.getString(&iter, _("Filename")),
                                     resultView.getString(&iter, _("Path")),
                                     resultView.getValue<int64_t>(&iter, "Real Size"),
                                     resultView.getString(&iter, _("TTH")));

            if (firstTTH)
            {
                tth = resultView.getString(&iter, _("TTH"));
                firstTTH = false;
                hasTTH = true;
            }
            else if (hasTTH)
            {
                if (tth.empty() || tth != resultView.getString(&iter, _("TTH")))
                    hasTTH = false; // Can't break here since we have to free all the paths
            }
        }
        gtk_tree_path_free(path);
    }
    g_list_free(list);

    if (hasTTH)
    {
        StringList targets = QueueManager::getInstance()->getTargets(TTHValue(tth));

        if (!targets.empty())
        {
            menuItem = gtk_separator_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadMenu")), menuItem);
            for (auto& i : targets)
            {
                menuItem = gtk_menu_item_new_with_label(i.c_str());
                g_signal_connect(menuItem, "activate", G_CALLBACK(onDownloadToMatchClicked_gui), (gpointer)this);
                gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadMenu")), menuItem);
            }
        }
    }

    // Build "Download whole directory to..." submenu

    spl.clear();
    spl = FavoriteManager::getInstance()->getFavoriteDirs();
    if (!spl.empty())
    {
        for (auto& i : spl)
        {
            menuItem = gtk_menu_item_new_with_label(i.second.c_str());
            g_object_set_data_full(G_OBJECT(menuItem), "fav", g_strdup(i.first.c_str()), g_free);
            g_signal_connect(menuItem, "activate", G_CALLBACK(onDownloadFavoriteDirClicked_gui), (gpointer)this);
            gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadDirMenu")), menuItem);
        }
        menuItem = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadDirMenu")), menuItem);
    }

    menuItem = gtk_menu_item_new_with_label(_("Browse..."));
    g_signal_connect(menuItem, "activate", G_CALLBACK(onDownloadDirToClicked_gui), (gpointer)this);
    gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadDirMenu")), menuItem);

    // Build user command menu
    userCommandMenu->buildMenu_gui();

#if GTK_CHECK_VERSION(3,22,0)
    gtk_menu_popup_at_pointer(GTK_MENU(getWidget("mainMenu")),NULL);
#else
    gtk_menu_popup(GTK_MENU(getWidget("mainMenu")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
    gtk_widget_show_all(getWidget("mainMenu"));
}

void Search::download_gui(const string &target)
{
    if (target.empty() || gtk_tree_selection_count_selected_rows(selection) <= 0)
        return;

    GtkTreeIter iter;
    GtkTreePath *path;
    GroupType groupBy = (GroupType)gtk_combo_box_get_active(GTK_COMBO_BOX(getWidget("comboboxGroupBy")));
    GList *list = gtk_tree_selection_get_selected_rows(selection, NULL);
    typedef Func6<Search, string, string, string, int64_t, string, string> F6;

    for (GList *i = list; i; i = i->next)
    {
        path = (GtkTreePath *)i->data;
        if (gtk_tree_model_get_iter(sortedFilterModel, &iter, path))
        {
            bool parent = gtk_tree_model_iter_has_child(sortedFilterModel, &iter);
            string filename = resultView.getString(&iter, _("Filename"));

            do
            {
                if (!gtk_tree_model_iter_has_child(sortedFilterModel, &iter))
                {
                    // User parent filename when grouping by TTH to avoid downloading the same file multiple times
                    if (groupBy != TTH || resultView.getString(&iter, _("Type")) == _("Directory"))
                    {
                        filename = resultView.getString(&iter, _("Path"));
                        filename += resultView.getString(&iter, _("Filename"));
                    }

                    string cid = resultView.getString(&iter, "CID");
                    int64_t size = resultView.getValue<int64_t>(&iter, "Real Size");
                    string tth = resultView.getString(&iter, _("TTH"));
                    string hubUrl = resultView.getString(&iter, "Hub URL");
                    F6 *func = new F6(this, &Search::download_client, target, cid, filename, size, tth, hubUrl);
                    WulforManager::get()->dispatchClientFunc(func);
                }
            }
            while (parent && WulforUtil::getNextIter_gui(sortedFilterModel, &iter, true, false));
        }
        gtk_tree_path_free(path);
    }
    g_list_free(list);
}
