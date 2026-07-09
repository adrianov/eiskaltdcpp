/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "MainAppCli.h"

#include "MainWindow.h"
#include "ShareIndex.h"

#include <QObject>

#include <cstdlib>
#include <iostream>

void parseCmdLine(const QStringList &args){
    for (const auto &arg : args){
        if (arg == "-h" || arg == "--help"){
            About().printHelp();

            exit(0);
        }
        else if (arg == "-V" || arg == "--version"){
            About().printVersion();

            exit(0);
        }
#ifdef USE_QT_SQLITE
        else if (arg == "--share-index-smoke"){
            QString err;
            if (!ShareIndex::smokeCheck(&err)) {
                std::cerr << "ShareIndex smoke failed: " << err.toStdString() << std::endl;
                exit(1);
            }
            std::cout << "ShareIndex smoke OK" << std::endl;
            exit(0);
        }
#endif
    }
}
