/*
 * Copyright (C) 2010 cologic, ne5@parsoma.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Script file / chunk evaluation for ScriptManager.
 */

#include "stdinc.h"

#include "ScriptManager.h"
#include "Util.h"
#include "LogManager.h"

namespace dcpp {

void ScriptInstance::EvaluateChunk(const string& chunk) {
    Lock l(cs);
    if (!L)
        return;
    lua_dostring(L, chunk.c_str());
}

void ScriptInstance::EvaluateFile(const string& fn) {
    Lock l(cs);
    if (!L)
        return;

    string script_full_name;
    if(Util::fileExists(fn)) {
        script_full_name = fn;
    } else {
        string test_path_0;
        string test_path_1;
#ifdef _WIN32
        test_path_0 = Text::utf8ToAcp(Util::getPath(Util::PATH_USER_CONFIG)) + "luascripts" + PATH_SEPARATOR + fn;
        test_path_1 = Text::utf8ToAcp(Util::getPath(Util::PATH_GLOBAL_CONFIG)) + "resources" + PATH_SEPARATOR + "luascripts" + PATH_SEPARATOR + fn;

        if(Util::fileExists(test_path_0))
            script_full_name = test_path_0;
        else if(Util::fileExists(test_path_1))
            script_full_name = test_path_1;
        else {
            LogManager::getInstance()->message("File '" + fn + "' not found!");
            dcdebug("File '%s' not found!\n",fn.c_str());
            return;
        }
#else
        test_path_0 = Text::utf8ToAcp(Util::getPath(Util::PATH_USER_CONFIG)) + "luascripts" + PATH_SEPARATOR + fn;
        test_path_1 = string(_DATADIR) + PATH_SEPARATOR + "luascripts" + PATH_SEPARATOR + fn;
#ifdef __linux
        string test_path_2;
        char result[PATH_MAX];
        const ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        if (count != -1) {
            test_path_2 = Util::getFilePath(string(result)) + "/../../";
        }
        test_path_2 = test_path_2 + string(_DATADIR) + PATH_SEPARATOR + "luascripts" + PATH_SEPARATOR + fn;
#endif

        if(Util::fileExists(test_path_0))
            script_full_name = test_path_0;
        else if(Util::fileExists(test_path_1))
            script_full_name = test_path_1;
#ifdef __linux
        else if(Util::fileExists(test_path_2))
            script_full_name = test_path_2;
#endif
        else {
            LogManager::getInstance()->message("File '" + fn + "' not found!");
            printf("File '%s' not found!\n",fn.c_str()); fflush(stdout);
            return;
        }
#endif
    }
    lua_dofile(L, script_full_name.c_str());
}

} // namespace dcpp
