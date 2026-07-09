/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifdef FORCE_XDG

#include "MainAppXdg.h"

#include "dcpp/stdinc.h"
#include "dcpp/forward.h"
#include "dcpp/Text.h"

#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QRegExp>
#include <QTextStream>

#include <cstdio>
#include <cstdlib>
#include <exception>

namespace {

void copy(const QDir &from, const QDir &to){
    if (!from.exists() || to.exists())
        return;

    QString to_path = to.absolutePath();
    QString from_path = from.absolutePath();

    if (!to_path.endsWith(QDir::separator()))
        to_path += QDir::separator();

    if (!from_path.endsWith(QDir::separator()))
        from_path += QDir::separator();

    for (const auto &s : from.entryList(QDir::Dirs)){
        QDir new_dir(to_path+s);

        if (new_dir.exists())
            continue;
        else{
            if (!new_dir.mkpath(new_dir.absolutePath()))
                continue;

            copy(QDir(from_path+s), new_dir);
        }
    }

    for (const auto &f : from.entryList(QDir::Files)){
        QFile orig(from_path+f);

        if (!orig.copy(to_path+f))
            continue;
    }
}

} // namespace

void migrateConfig(){
    const char* home_ = getenv("HOME");
    dcpp::string home = home_ ? dcpp::Text::toUtf8(home_) : "/tmp/";
    dcpp::string old_config = home + "/.eiskaltdc++/";

    const char *xdg_config_home_ = getenv("XDG_CONFIG_HOME");
    dcpp::string xdg_config_home = xdg_config_home_? dcpp::Text::toUtf8(xdg_config_home_) : (home+"/.config");
    dcpp::string new_config = xdg_config_home + "/eiskaltdc++/";

    if (!QDir().exists(old_config.c_str()) || QDir().exists(new_config.c_str())){
        if (!QDir().exists(new_config.c_str())){
            old_config = _DATADIR + dcpp::string("/config/");

            if (!QDir().exists(old_config.c_str()))
                return;
        }
        else
            return;
    }

    try{
        printf("Migrating to XDG paths...\n");

        copy(QDir(old_config.c_str()), QDir(new_config.c_str()));

        QFile orig(new_config.c_str()+QString("DCPlusPlus.xml"));
        QFile new_file(new_config.c_str()+QString("DCPlusPlus.xml.new"));

        if (!(orig.open(QIODevice::ReadOnly | QIODevice::Text) && new_file.open(QIODevice::WriteOnly | QIODevice::Text))){
            orig.close();
            new_file.close();

            printf("Migration failed.\n");

            return;
        }

        QTextStream rstream(&orig);
        QTextStream wstream(&new_file);

        QRegExp replace_str("/(\\S+)/\\.eiskaltdc\\+\\+/");
        QString line = "";

        while (!rstream.atEnd()){
            line = rstream.readLine();

            line.replace(replace_str, QString(new_config.c_str()));

            wstream << line << "\n";
        }

        wstream.flush();

        orig.close();
        new_file.close();

        orig.remove();
        new_file.rename(orig.fileName());

        printf("Ok. Migrated.\n");
    }
    catch(const std::exception&){
        printf("Migration failed.\n");
    }
}

#endif
