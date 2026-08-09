cmake_minimum_required (VERSION 3.10)
project (eiskaltdcpp-qt)

include_directories(${PROJECT_BINARY_DIR})

find_package (Qt5Core          5.15.0 REQUIRED)
find_package (Qt5Gui           5.15.0 REQUIRED)
find_package (Qt5Widgets       5.15.0 REQUIRED)
find_package (Qt5Xml           5.15.0 REQUIRED)
find_package (Qt5Network       5.15.0 REQUIRED)
# This is temporary workaround which fixes build with Qt5 on Travis CI:
find_package (Qt5LinguistTools       REQUIRED)
find_package (Qt5Multimedia    5.15.0 REQUIRED)
find_package (Qt5Concurrent    5.15.0 REQUIRED)

if (USE_QT_QML)
    find_package (Qt5Declarative 5.15.0 REQUIRED)
    find_package (Qt5XmlPatterns 5.15.0 REQUIRED)
    add_definitions(-DUSE_QML)
    set (USE_JS ON)
    message (STATUS "Building with Qt Declarative UI support")
else (USE_QT_QML)
    message (STATUS "Building without Qt Declarative UI support")
endif (USE_QT_QML)

if (APPLE OR HAIKU)
    set (DBUS_NOTIFY OFF)
endif ()

if (DBUS_NOTIFY)
    find_package (Qt5DBus 5.15.0 REQUIRED)
    add_definitions (-DDBUS_NOTIFY)
    message (STATUS "Building with Qt DBus support")
else (DBUS_NOTIFY)
    message (STATUS "Building without Qt DBus support")
endif (DBUS_NOTIFY)

if (USE_JS)
    find_package (Qt5Script 5.15.0 REQUIRED)
    add_definitions (-DUSE_JS)
    message (STATUS "Building with Qt Script support")
else (USE_JS)
    message (STATUS "Building without Qt Script support")
endif (USE_JS)

if (USE_QT_SQLITE)
    find_package (Qt5Sql 5.15.0 REQUIRED)
    find_package (DuckDB REQUIRED)
    add_definitions (-DUSE_QT_SQLITE)
    message (STATUS "Building with Qt SQLite (FinishedTransfers) + DuckDB ShareIndex")
else (USE_QT_SQLITE)
    message (STATUS "Building without Qt SQLite / DuckDB ShareIndex")
endif (USE_QT_SQLITE)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SRCS)

# Explicit modules (also from aux_source_directory). Keeps splits visible; appends if glob is stale.
set(QT_EXTRA_SOURCES
    src/AppThemeControls.cpp
    src/AppThemeCombo.cpp
    src/AppThemeInputs.cpp
    src/AppThemeChatDoc.cpp
    src/AppThemeHighlight.cpp
    src/ChatSearchBar.cpp
    src/FileBrowserItem.cpp
    src/FileBrowserModelData.cpp
    src/FileBrowserModelFetch.cpp
    src/FileBrowserModelMedia.cpp
    src/FileBrowserModelPath.cpp
    src/MediaEnrichQueue.cpp
    src/filebrowser/FilterMatch.cpp
    src/filebrowser/ListFilter.cpp
    src/filebrowser/ListFilterProxy.cpp
    src/filebrowser/FileBrowserFilterProxy.cpp
    src/FileBrowserModelSort.cpp
    src/PublicHubItem.cpp
    src/PublicHubModelData.cpp
    src/PublicHubModelSort.cpp
    src/PublicHubsActions.cpp
    src/PublicHubsFilter.cpp
    src/queuedusers/QueuedUserItem.cpp
    src/queuedusers/QueuedUserTree.cpp
    src/queuedusers/QueuedUsers.cpp
    src/queuedusers/QueuedUsersModel.cpp
    src/FavoriteUsersActions.cpp
    src/FavoriteHubItem.cpp
    src/FavoriteHubModelMut.cpp
    src/FavoriteHubModelSort.cpp
    src/FinishedTransfers.cpp
    src/FinishedTransfersInit.cpp
    src/FinishedTransfersItem.cpp
    src/FinishedTransfersListener.cpp
    src/FinishedTransfersLoad.cpp
    src/FinishedTransfersMenu.cpp
    src/FinishedTransfersOpen.cpp
    src/FinishedTransfersModelMut.cpp
    src/FinishedTransfersModelView.cpp
    src/FinishedTransfersParams.cpp
    src/FinishedTransfersDb.cpp
    src/FinishedTransfersDelete.cpp
    src/FinishedTransfersProxy.cpp
    src/FinishedTransfersSlots.cpp
    src/hubframe/HubChatCmd.cpp
    src/hubframe/HubChatCompose.cpp
    src/hubframe/HubFrameChatMenu.cpp
    src/hubframe/HubFrameClient.cpp
    src/hubframe/HubFrameLinkMagnet.cpp
    src/hubframe/HubFrameLinkParser.cpp
    src/hubframe/HubFrameMenu.cpp
    src/hubframe/HubFrameMenuExec.cpp
    src/hubframe/HubFrameMsg.cpp
    src/hubframe/HubFrameUserList.cpp
    src/hubframe/HubFrameUsers.cpp
    src/hubframe/HubPaneLayout.cpp
    src/PmSpamFilter.cpp
    src/PMWindowEvents.cpp
    src/MainAppCli.cpp
    src/MainAppRun.cpp
    src/MainAppShareIndex.cpp
    src/MainAppUnix.cpp
    src/MainAppXdg.cpp
    src/mainwindow/MainWindow.cpp
    src/mainwindow/MainWindowArena.cpp
    src/mainwindow/MainWindowAbout.cpp
    src/mainwindow/MainWindowStatus.cpp
    src/mainwindow/MainWindowTools.cpp
    src/mainwindow/actions/WindowActions.cpp
    src/mainwindow/actions/ActionCatalog.cpp
    src/mainwindow/actions/MenuLabels.cpp
    src/mainwindow/WindowChrome.cpp
    src/mainwindow/WindowPlace.cpp
    src/mainwindow/WindowLife.cpp
    src/mainwindow/WindowSetup.cpp
    src/mainwindow/WindowShare.cpp
    src/mainwindow/WindowTools.cpp
    src/statusbar/StatusHud.cpp
    src/statusbar/StatusStrip.cpp
    src/statusbar/FreeSpaceBar.cpp
    src/statusbar/HashProgressBar.cpp
    src/statusbar/StatusBarLogLabel.cpp
    src/SearchFrameContext.cpp
    src/SearchFrameContextDl.cpp
    src/SearchFrameContextMore.cpp
    src/SearchFrameContextUser.cpp
    src/SearchFrameActions.cpp
    src/SearchFrameHistory.cpp
    src/SearchFrameInit.cpp
    src/SearchFrameLife.cpp
    src/SearchFrameFilter.cpp
    src/SearchFrameQueue.cpp
    src/SearchFrameLocal.cpp
    src/SearchFrameMenu.cpp
    src/SearchFrameStart.cpp
    src/SearchFrameUi.cpp
    src/SearchFrameTimer.cpp
    src/SearchFrameMenuExec.cpp
    src/SearchFrameParams.cpp
    src/SearchFrameMedia.cpp
    src/search/SearchLocalPath.cpp
    src/SearchFileTypes.cpp
    src/search/SearchItemSort.cpp
    src/SearchModelAdd.cpp
    src/SearchModelMedia.cpp
    src/SearchModelData.cpp
    src/SearchModelMutate.cpp
    src/SearchModelNav.cpp
    src/SearchProxyModel.cpp
    src/search/SearchItem.cpp
    src/ReciprocalList.cpp
    src/ReciprocalListPeer.cpp
    src/ShareIndexListListener.cpp
    src/settings/SettingsSharing.cpp
    src/settings/ShareDirModel.cpp
    src/settings/ShareDirsPane.cpp
    src/settings/SimpleShareTree.cpp
    src/SettingsConnectionInit.cpp
    src/SettingsConnectionPorts.cpp
    src/SettingsConnectionSave.cpp
    src/SettingsGUIColor.cpp
    src/SettingsGUIInit.cpp
    src/SettingsGUIInitBasic.cpp
    src/SettingsGUIInitColor.cpp
    src/SettingsGUISave.cpp
    src/ToolBarTabs.cpp
    src/ToolBarRedraw.cpp
    src/TabFrameNav.cpp
    src/TabFrameActions.cpp
    src/TabFrameWidgets.cpp
    src/SideBarDelegate.cpp
    src/SideBarView.cpp
    src/SideBarViewActions.cpp
    src/SideBarModelMutate.cpp
    src/SideBarModelHistory.cpp
    src/SideBarModelData.cpp
    src/TransferViewModelParents.cpp
    src/appicon/AppIconsLoad.cpp
    src/appicon/AppIconsScale.cpp
    src/appicon/FileTypeIcon.cpp
    src/appicon/FileTypeIconMap.cpp
    src/appicon/FileTypeIconExtra.cpp
    src/appicon/WulforUtilAppIcons.cpp
    src/WulforUtilPaths.cpp
    src/WulforUtilUserIcon.cpp
    src/WulforUtilUrl.cpp
    src/WulforUtilUserCmd.cpp
    src/WulforUtilNet.cpp
    src/WulforUtilHeader.cpp
    src/WulforUtilMagnet.cpp
    src/WulforUtilText.cpp
    src/treeheader/ColumnContentSpan.cpp
    src/treeheader/HeaderColumnFit.cpp
    src/treeheader/TreeHeaderAutosize.cpp
    src/treeheader/WulforTreeHeader.cpp
    src/WulforSettingsDefaults.cpp
    src/WulforSettingsOldMaps.cpp
    src/WulforSettingsI18n.cpp
    src/ShareIndex.cpp
    src/shareindex/ShareIndexStore.cpp
    src/ShareIndexIngest.cpp
    src/ShareIndexIngestWrite.cpp
    src/ShareIndexInsert.cpp
    src/ShareIndexEnsure.cpp
    src/ShareIndexMigrate.cpp
    src/ShareIndexQueue.cpp
    src/ShareIndexQueueCore.cpp
    src/ShareIndexQueueMatch.cpp
    src/ShareIndexRowWrite.cpp
    src/ShareIndexEnqueue.cpp
    src/ShareIndexSchema.cpp
    src/ShareIndexSearch.cpp
    src/ShareIndexSearchQuery.cpp
    src/ShareIndexSearchRows.cpp
    src/ShareIndexUsers.cpp
    src/ShareIndexMedia.cpp
    src/ShareIndexCap.cpp
    src/ShareIndexVacuum.cpp
    src/ShareIndexStats.cpp
    src/ShareIndexSmoke.cpp
    src/ShareIndexSmokeWrite.cpp
    src/ShareIndexSmokeMigrate.cpp
    src/ShareIndexSmokeSearch.cpp
    src/ShareIndexSmokeUsers.cpp
    src/ShareIndexUpsert.cpp
    src/ShareIndexWalk.cpp
    src/ShareBrowserContext.cpp
    src/ShareBrowserContextMore.cpp
    src/ShareBrowserNav.cpp
    src/ShareBrowserSelect.cpp
    src/sharebrowser/ShareFolderList.cpp
    src/sharebrowser/ShareMediaState.cpp
    src/sharebrowser/ListingMediaIndex.cpp
    src/sharebrowser/ShareBrowserList.cpp
    src/sharebrowser/ShareBrowserMedia.cpp
    src/sharebrowser/ShareBrowserMenu.cpp
    src/sharebrowser/AsyncRunner.cpp
    src/fb2epub/EpubPack.cpp
    src/fb2epub/Fb2EpubExport.cpp
    src/fb2epub/Fb2EpubConverter.cpp
    src/fb2epub/Fb2EpubMetaParse.cpp
    src/fb2epub/Fb2EpubNav.cpp
    src/fb2epub/Fb2EpubOpf.cpp
    src/fb2epub/Fb2EpubParser.cpp
    src/fb2epub/Fb2EpubParserBody.cpp
    src/fb2epub/Fb2EpubParserBodyExtra.cpp
    src/fb2epub/Fb2EpubParserBodyStart.cpp
    src/fb2epub/Fb2EpubParserEnd.cpp
    src/fb2epub/Fb2EpubParserFootnotes.cpp
    src/fb2epub/Fb2EpubParserImages.cpp
    src/fb2epub/Fb2EpubParserMetaEnd.cpp
    src/fb2epub/Fb2EpubParserMetaStart.cpp
    src/fb2epub/Fb2EpubParserStart.cpp
    src/fb2epub/Fb2EpubText.cpp
    src/fb2epub/Fb2EpubWriter.cpp
    src/fb2epub/Fb2Format.cpp
    src/fb2epub/xml/SaxParser.cpp
    src/ShareBrowserActions.cpp
    src/ShareBrowserFilter.cpp
    src/ShareBrowserInit.cpp
    src/ShareBrowserSetup.cpp
    src/ShareBrowserContextUser.cpp
    src/ShareBrowserDelete.cpp
    src/DownloadQueueContext.cpp
    src/DownloadQueueContextMore.cpp
    src/DownloadQueueMenu.cpp
    src/DownloadQueueItems.cpp
    src/DownloadQueueListen.cpp
    src/DownloadQueueParams.cpp
    src/DownloadQueueIndex.cpp
    src/DownloadQueueConnect.cpp
    src/DownloadQueueModelData.cpp
    src/DownloadQueueModelMut.cpp
    src/DownloadQueueModelPath.cpp
    src/DownloadQueueModelSort.cpp
    src/transferdisplay/TransferDisplay.cpp

    src/TransferViewActions.cpp
    src/TransferViewConnectionListeners.cpp
    src/TransferViewDelegate.cpp
    src/TransferViewDownloadListeners.cpp
    src/transferrow/TransferViewItem.cpp
    src/TransferViewMenu.cpp
    src/TransferViewMenuCopy.cpp
    src/TransferViewMenuExec.cpp
    src/TransferViewModelLookup.cpp
    src/TransferViewModelMutate.cpp
    src/TransferViewModelAttach.cpp
    src/TransferViewModelNav.cpp
    src/TransferViewModelQueue.cpp
    src/TransferViewModelRemove.cpp
    src/TransferViewModelRemoveQueue.cpp
    src/transfergrace/TransferGrace.cpp
    src/transfergrace/TransferViewRemoveUtil.cpp
    src/TransferViewModelPrune.cpp
    src/TransferViewModelSort.cpp
    src/TransferViewParams.cpp
    src/TransferViewRemove.cpp
    src/TransferViewPath.cpp
    src/TransferViewMetrics.cpp
    src/TransferViewUploadListeners.cpp
    src/transfersession/TransferGroup.cpp
    src/transfersession/TransferSession.cpp
    src/transfersession/TransferSessionRow.cpp
    src/UserListItem.cpp
    src/UserListModelData.cpp
    src/UserListModelMatch.cpp
    src/UserListModelMutate.cpp
    src/UserListModelSort.cpp
)
foreach(_src ${QT_EXTRA_SOURCES})
    set(_path ${PROJECT_SOURCE_DIR}/${_src})
    if(NOT EXISTS ${_path})
        message(FATAL_ERROR "eiskaltdcpp-qt source missing: ${_src}")
    elseif(NOT "${_path}" IN_LIST SRCS)
        list(APPEND SRCS ${_path})
    endif()
endforeach()

if (NOT HAIKU)
    list (APPEND SRCS src/qtsingleapp/qtsinglecoreapplication.cpp)
endif (NOT HAIKU)

file (GLOB MOC_HDRS ${PROJECT_SOURCE_DIR}/src/*.h)
list (APPEND MOC_HDRS ${PROJECT_SOURCE_DIR}/src/statusbar/StatusBarLogLabel.h)
list (APPEND MOC_HDRS ${PROJECT_SOURCE_DIR}/src/transfergrace/TransferGrace.h)
list (APPEND MOC_HDRS
            ${PROJECT_SOURCE_DIR}/src/mainwindow/MainWindow.h
            ${PROJECT_SOURCE_DIR}/src/filebrowser/FileBrowserFilterProxy.h
            ${PROJECT_SOURCE_DIR}/src/filebrowser/ListFilterProxy.h
            ${PROJECT_SOURCE_DIR}/src/queuedusers/QueuedUsers.h
            ${PROJECT_SOURCE_DIR}/src/queuedusers/QueuedUsersModel.h
            ${PROJECT_SOURCE_DIR}/src/settings/SettingsSharing.h
            ${PROJECT_SOURCE_DIR}/src/settings/ShareDirModel.h
            ${PROJECT_SOURCE_DIR}/src/settings/ShareDirsPane.h
            ${PROJECT_SOURCE_DIR}/src/settings/SimpleShareTree.h)
list (REMOVE_ITEM MOC_HDRS
            ${PROJECT_SOURCE_DIR}/src/mainwindow/MainWindowPrivate.h
            ${PROJECT_SOURCE_DIR}/src/SearchFramePrivate.h
            ${PROJECT_SOURCE_DIR}/src/SearchFrameLocal.h
            ${PROJECT_SOURCE_DIR}/src/hubframe/HubFramePrivate.h
            ${PROJECT_SOURCE_DIR}/src/DownloadQueuePrivate.h)

if (NOT HAIKU AND NOT APPLE)
    list (APPEND MOC_HDRS
                src/qtsingleapp/qtsinglecoreapplication.h)
    list (REMOVE_ITEM MOC_HDRS
                ${PROJECT_SOURCE_DIR}/src/EiskaltApp_haiku.h)
    list (REMOVE_ITEM MOC_HDRS
                ${PROJECT_SOURCE_DIR}/src/EiskaltApp_mac.h)
elseif (HAIKU)
    list (REMOVE_ITEM MOC_HDRS
                ${PROJECT_SOURCE_DIR}/src/EiskaltApp.h)
    list (REMOVE_ITEM MOC_HDRS
                ${PROJECT_SOURCE_DIR}/src/EiskaltApp_mac.h)
elseif (APPLE)
    list (APPEND MOC_HDRS
                src/qtsingleapp/qtsinglecoreapplication.h)
    list (REMOVE_ITEM MOC_HDRS
                ${PROJECT_SOURCE_DIR}/src/EiskaltApp.h)
    list (REMOVE_ITEM MOC_HDRS
                ${PROJECT_SOURCE_DIR}/src/EiskaltApp_haiku.h)
endif (NOT HAIKU AND NOT APPLE)

file (GLOB UI_FORMS ${PROJECT_SOURCE_DIR}/ui/*.ui)

set (WIN_RESOURCES)
if (MINGW AND WIN32)
    add_custom_command (OUTPUT "${PROJECT_BINARY_DIR}/eiskaltdcpp-qt_rc.o"
                        COMMAND ${CMAKE_RC_COMPILER}
                                -I"${PROJECT_SOURCE_DIR}/"
                                -i "${PROJECT_SOURCE_DIR}/eiskaltdcpp-qt.rc"
                                -o "${PROJECT_BINARY_DIR}/eiskaltdcpp-qt_rc.o")
    set (WIN_RESOURCES ${PROJECT_BINARY_DIR}/eiskaltdcpp-qt_rc.o)
elseif (NOT MINGW AND WIN32)
    set (WIN_RESOURCES"${PROJECT_SOURCE_DIR}/eiskaltdcpp-qt.rc")
endif (MINGW AND WIN32)

if (NOT USE_ASPELL)
    message (STATUS "Building without Aspell support")
    set (LOCAL_ASPELL_DATA OFF)
elseif (USE_ASPELL)
    find_package (ASPELL REQUIRED)
    message (STATUS "Building with Aspell support")
endif (NOT USE_ASPELL)

if (NOT ASPELL_FOUND)
    list (REMOVE_ITEM SRCS ${PROJECT_SOURCE_DIR}/src/SpellCheck.cpp)
    list (REMOVE_ITEM MOC_HDRS ${PROJECT_SOURCE_DIR}/src/SpellCheck.h)
endif (NOT ASPELL_FOUND)

if (USE_JS)
    file (GLOB scriptengine_srcs ${PROJECT_SOURCE_DIR}/scriptengine/*.cpp)
    file (GLOB scriptengine_hdrs ${PROJECT_SOURCE_DIR}/scriptengine/*.h)
    list (APPEND SRCS
            ${scriptengine_srcs}
            src/codeeditor/codeeditor.cpp)
    list (APPEND MOC_HDRS
            ${scriptengine_hdrs}
            src/codeeditor/codeeditor.h)
endif (USE_JS)

if (NOT USE_JS)
    list (REMOVE_ITEM UI_FORMS
            ${PROJECT_SOURCE_DIR}/ui/UIScriptManager.ui
            ${PROJECT_SOURCE_DIR}/ui/UIDialogScriptConsole.ui)
    list (REMOVE_ITEM SRCS
            ${PROJECT_SOURCE_DIR}/src/ScriptManagerDialog.cpp)
    list (REMOVE_ITEM MOC_HDRS
            ${PROJECT_SOURCE_DIR}/src/ScriptManagerDialog.h)
endif (NOT USE_JS)

#Create moc files
# -I src so subdirectory headers (e.g. queuedusers/) resolve ArenaWidget and peers for moc.
QT5_WRAP_CPP (M_SRCS ${MOC_HDRS} OPTIONS -nn -I${PROJECT_SOURCE_DIR}/src)
#Create .h files from ui files
QT5_WRAP_UI (U_SRCS ${UI_FORMS})

set (QT_LRELEASE_EXECUTABLE ${Qt5_LRELEASE_EXECUTABLE})
get_target_property(QT_RCC_EXECUTABLE Qt5::rcc LOCATION)

#Create translations
file (MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/translations/)
foreach (LANGUAGE ${LANGUAGES})
        set (TS ${PROJECT_SOURCE_DIR}/translations/${LANGUAGE}.ts)
        set (QM ${PROJECT_BINARY_DIR}/translations/${LANGUAGE}.qm)
        list (APPEND TRANSLATIONS ${TS})
        list (APPEND TRANSLATIONS_BINARY ${QM})
        add_custom_command (OUTPUT "${QM}"
                            DEPENDS "${TS}"
                            COMMAND ${QT_LRELEASE_EXECUTABLE}
                            ARGS "${TS}" -qm "${QM}")
endforeach ()

if (WIN32 OR APPLE OR INSTALL_QT_TRANSLATIONS OR BUILD_STATIC)
    # Qt translations
    get_target_property (QtCore_location Qt5::Core LOCATION)
    get_filename_component (QtCore_DIR "${QtCore_location}" PATH)
    if (WIN32)
        set (QT_ROOT_DIR "${QtCore_DIR}/..")
    elseif (APPLE)
        set (QT_ROOT_DIR "${QtCore_DIR}/../..")
    elseif (BUILD_STATIC)
        set (QT_ROOT_DIR "${QtCore_DIR}/..")
    elseif (INSTALL_QT_TRANSLATIONS AND NOT BUILD_STATIC)
        set (QT_ROOT_DIR "/usr/share/qt5")
    endif ()
    set (QT_TRANSLATIONS_DIR "${QT_ROOT_DIR}/translations/")
    file (GLOB QT_TRANSLATIONS "${QT_TRANSLATIONS_DIR}/qt_*.qm")
    foreach (FILE ${QT_TRANSLATIONS})
        if (NOT FILE MATCHES "_help_")
            list (APPEND TRANSLATIONS_BINARY ${FILE})
        endif ()
    endforeach ()
    file (GLOB QT_TRANSLATIONS "${QT_TRANSLATIONS_DIR}/qtbase_*.qm")
    list (APPEND TRANSLATIONS_BINARY ${QT_TRANSLATIONS})
endif ()

add_custom_target(${PROJECT_NAME}_tr ALL DEPENDS ${TRANSLATIONS_BINARY})

set (DEFAULT_QRC_FILE ${PROJECT_SOURCE_DIR}/icons/appl/default/default.qrc)

if (UNIX AND NOT APPLE AND NOT HAIKU)
    if(COMPRESS_MANPAGES)
        execute_process (OUTPUT_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.1.gz
                         COMMAND "${GZIP_CMD}" -9
                         INPUT_FILE ${PROJECT_NAME}.1
                         WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/)
        install (FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.1.gz DESTINATION ${MAN_DIR})
    else(COMPRESS_MANPAGES)
        install (FILES ${PROJECT_NAME}.1 DESTINATION ${MAN_DIR})
    endif(COMPRESS_MANPAGES)
endif ()

if (WIN32 OR APPLE)
    install (FILES manual.${PROJECT_NAME}.html DESTINATION "${CLIENT_DOCS_DIR}" RENAME manual.${PROJECT_NAME_GLOBAL}.html)
endif ()

execute_process (COMMAND ${QT_RCC_EXECUTABLE} -binary  ${DEFAULT_QRC_FILE}
                                              -o ${PROJECT_BINARY_DIR}/default.rcc
                 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/)

include_directories (${Qt5Core_INCLUDE_DIRS}
                     ${Qt5Gui_INCLUDE_DIRS}
                     ${Qt5Widgets_INCLUDE_DIRS}
                     ${Qt5Xml_INCLUDE_DIRS}
                     ${Qt5Multimedia_INCLUDE_DIRS}
                     ${Qt5Concurrent_INCLUDE_DIRS}
                     ${Qt5Declarative_INCLUDE_DIRS}
                     ${Qt5XmlPatterns_INCLUDE_DIRS}
                     ${Qt5DBus_INCLUDE_DIRS}
                     ${Qt5Script_INCLUDE_DIRS}
                     ${Qt5Sql_INCLUDE_DIRS}
                     ${Qt5Network_INCLUDE_DIRS})
list (APPEND LIBS ${Qt5Core_LIBRARIES}
                  ${Qt5Gui_LIBRARIES}
                  ${Qt5Widgets_LIBRARIES}
                  ${Qt5Xml_LIBRARIES}
                  ${Qt5Multimedia_LIBRARIES}
                  ${Qt5Concurrent_LIBRARIES}
                  ${Qt5Declarative_LIBRARIES}
                  ${Qt5XmlPatterns_LIBRARIES}
                  ${Qt5DBus_LIBRARIES}
                  ${Qt5Script_LIBRARIES}
                  ${Qt5Sql_LIBRARIES}
                  ${Qt5Network_LIBRARIES})

if (USE_QT_SQLITE)
    include_directories (${DuckDB_INCLUDE_DIRS})
    list (APPEND LIBS ${DuckDB_LIBRARIES})
endif ()

if (USE_MEDIAINFO AND MediaInfo_FOUND)
    list (APPEND LIBS ${MediaInfo_LIBRARIES})
endif ()

include_directories (${PROJECT_SOURCE_DIR}
                     ${PROJECT_SOURCE_DIR}/src
                     ${PROJECT_SOURCE_DIR}/src/mainwindow
                     ${PROJECT_SOURCE_DIR}/src/fb2epub
                     ${PROJECT_SOURCE_DIR}/scriptengine
                     ${PROJECT_SOURCE_DIR}/..
                     ${GETTEXT_INCLUDE_DIR}
                     ${ZLIB_INCLUDE_DIR})
if (NOT APPLE)
    list (APPEND LIBS ${ICONV_LIBRARIES})
endif ()

if (BUILD_STATIC AND USE_QT5)
    if (WIN32)
        find_library (QT5FontDatabaseSupport Qt5FontDatabaseSupport)
        find_library (QT5EventDispatcherSupport Qt5EventDispatcherSupport)
        find_library (QT5ThemeSupport Qt5ThemeSupport)
        find_library (QT5AccessibilitySupport Qt5AccessibilitySupport)
        find_library (QT5WindowsUIAutomationSupport Qt5WindowsUIAutomationSupport)
        list (INSERT LIBS 0 Qt5::QWindowsVistaStylePlugin)
        if (DBUS_NOTIFY)
            list (APPEND LIBS Qt5::DBus -ldbus-1)
        endif (DBUS_NOTIFY)
        list (APPEND LIBS Qt5::QWindowsIntegrationPlugin
                          ${QT5FontDatabaseSupport}
                          ${QT5EventDispatcherSupport}
                          ${QT5ThemeSupport}
                          ${QT5AccessibilitySupport}
                          ${QT5WindowsUIAutomationSupport}
                          Qt5::QSQLiteDriverPlugin
                          Qt5::Sql
                          Qt5::QWindowsAudioPlugin
                          Qt5::Multimedia
                          ${PCRE2_LIBRARIES}
                          -lharfbuzz
                          -lwinmm
                          -limm32
                          -lsqlite3
                          -ljpeg
                          -lpng
                          -lopengl32
                          -lwtsapi32
                          -lwinmm
                          -lcrypt32
                          -lstrmiids
                          -lfreetype
             )
    elseif (UNIX AND NOT APPLE AND NOT HAIKU)
        add_definitions (-DSQLITE_ENABLE_COLUMN_METADATA=1)
        find_library (QT5FontDatabaseSupport Qt5FontDatabaseSupport)
        find_library (QT5EventDispatcherSupport Qt5EventDispatcherSupport)
        find_library (QT5ThemeSupport Qt5ThemeSupport)
        find_library (QT5ServiceSupport Qt5ServiceSupport)
        find_library (QT5AccessibilitySupport Qt5AccessibilitySupport)
        find_library (QT5EdidSupport Qt5EdidSupport)
        find_library (QT5XcbQpa Qt5XcbQpa)
        find_library (XcbStatic xcb-static)
        if (DBUS_NOTIFY)
            set (QT5DBus Qt5::DBus)
        else (DBUS_NOTIFY)
            set (QT5DBus "")
        endif (DBUS_NOTIFY)
        list (APPEND LIBS Qt5::QXcbIntegrationPlugin
                          ${QT5XcbQpa}
                          ${QT5FontDatabaseSupport}
                          ${QT5EventDispatcherSupport}
                          ${QT5ThemeSupport}
                          ${QT5ServiceSupport}
                          ${QT5AccessibilitySupport}
                          ${QT5EdidSupport}
                          Qt5::QSQLiteDriverPlugin
                          Qt5::Sql
                          Qt5::Multimedia
                          ${QT5DBus}
                          ${XcbStatic}
                          ${PCRE2_LIBRARIES}
                          -lsqlite3
                          -ljpeg
                          -lpng
                          -lfontconfig
                          -lfreetype
                          -luuid
                          -lexpat
                          -lSM
                          -lICE
                          -lGL
                          -lm
                          -ldl
                          -ldrm
                          -lxkbcommon
                          -lxkbcommon-x11
                          -lXdamage
                          -lXfixes
                          -lX11-xcb
                          -lxcb-glx
                          -lxcb-dri2
                          -lXxf86vm
                          -lXext
                          -lX11
                          -lxcb
                          -lXau
                          -lXdmcp
             )
    endif (WIN32)
endif (BUILD_STATIC AND USE_QT5)

if (WIN32)
    if (OPENSSL_MSVC)
        list (APPEND SRCS ../windows/applink.c)
    endif (OPENSSL_MSVC)
endif (WIN32)

if (APPLE)
    find_library (COCOA_LIBRARY Cocoa)
    find_library (CORESERVICES_LIBRARY CoreServices)
    list (APPEND LIBS ${COCOA_LIBRARY} ${CORESERVICES_LIBRARY})
endif (APPLE)

if (FREE_SPACE_BAR_C)
    add_definitions (-DFREE_SPACE_BAR_C)
endif (FREE_SPACE_BAR_C)

if (USE_ASPELL)
    include_directories (${ASPELL_INCLUDE_DIR})
    list (APPEND LIBS ${ASPELL_LIBRARIES})
    add_definitions (-DUSE_ASPELL ${ASPELL_DEFINITIONS})
    if (LOCAL_ASPELL_DATA)
        add_definitions (-DLOCAL_ASPELL_DATA)
    endif (LOCAL_ASPELL_DATA)
endif (USE_ASPELL)

if (LUA_SCRIPT)
    include_directories (${LUA_INCLUDE_DIR})
    list (APPEND LIBS ${LUA_LIBRARIES})
endif (LUA_SCRIPT)

if (USE_MINIUPNP)
    set_property(SOURCE
        ${PROJECT_SOURCE_DIR}/src/SettingsConnection.cpp
        ${PROJECT_SOURCE_DIR}/src/SettingsConnectionInit.cpp
        ${PROJECT_SOURCE_DIR}/src/SettingsConnectionSave.cpp
        PROPERTY COMPILE_DEFINITIONS USE_MINIUPNP)
endif()

if (FORCE_XDG)
   set_property(SOURCE ${PROJECT_SOURCE_DIR}/src/main.cpp
                        ${PROJECT_SOURCE_DIR}/src/MainAppXdg.cpp
                PROPERTY COMPILE_DEFINITIONS FORCE_XDG )
endif (FORCE_XDG)

if (HAVE_IFADDRS_H)
  set_property(SOURCE ${PROJECT_SOURCE_DIR}/src/WulforUtil.cpp
               PROPERTY COMPILE_DEFINITIONS HAVE_IFADDRS_H APPEND)
endif (HAVE_IFADDRS_H)

if (ENABLE_STACKTRACE)
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/src/main.cpp
                 PROPERTY COMPILE_DEFINITIONS ENABLE_STACKTRACE)
endif (ENABLE_STACKTRACE)

if (APPLE)
    set (MACOSICON "${PROJECT_SOURCE_DIR}/../data/icons/icon_1024x1024.icns")
    set (MACOSICONINBUNDLE "icon_1024x1024.icns")
    # Set how it shows up in the Info.plist file
    set (MACOSX_BUNDLE_ICON_FILE ${MACOSICONINBUNDLE})
    # Set where in the bundle to put the icns file
    set_source_files_properties(${MACOSICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    # Include the icns file in the target
    list (APPEND SRCS ${MACOSICON})
endif (APPLE)

add_executable (${PROJECT_NAME} MACOSX_BUNDLE WIN32 src/main.cpp
                ${M_SRCS}
                ${U_SRCS}
                ${SRCS}
                ${HDRS}
                ${MOC_HDRS}
                ${TRANSLATIONS}
                ${WIN_RESOURCES}
                translations)

add_definitions (-DCLIENT_DATA_DIR="${CLIENT_DATA_DIR}")
add_definitions (-DCLIENT_SOUNDS_DIR="${CLIENT_SOUNDS_DEST}")
add_definitions (-DCLIENT_TRANSLATIONS_DIR="${CLIENT_TRANSLATIONS_DEST}")

if (DEBUG_QT_UI)
    add_definitions (-D_DEBUG_QT_UI)
    message (STATUS "Enabled debug output from eiskaltdcpp-qt!")
endif (DEBUG_QT_UI)
if (DEBUG_IPFILTER)
    add_definitions (-D_DEBUG_IPFILTER)
endif (DEBUG_IPFILTER)

if (USE_JS)
    if (UNIX AND NOT APPLE AND NOT HAIKU)
        set (CLIENT_SCRIPTS_DEST "${CLIENT_DATA_DIR}/qt/qtscripts")
    else ()
        set (CLIENT_SCRIPTS_DEST "${CLIENT_DATA_DIR}/qtscripts")
    endif ()
    add_definitions (-DCLIENT_SCRIPTS_DIR="${CLIENT_SCRIPTS_DEST}")
endif (USE_JS)

if (WIN32)
    add_definitions (-DCLIENT_ICONS_DIR="${CLIENT_DATA_DIR}/icons")
    add_definitions (-DCLIENT_RES_DIR="${CLIENT_DATA_DIR}/client-res")
elseif (HAIKU)
    add_definitions (-DCLIENT_ICONS_DIR="icons")
    add_definitions (-DCLIENT_RES_DIR="client-res")
elseif (APPLE)
    add_definitions (-DCLIENT_ICONS_DIR="icons")
    add_definitions (-DCLIENT_RES_DIR="client-res")
else ()
    add_definitions (-DCLIENT_ICONS_DIR="${CLIENT_DATA_DIR}/qt/icons")
    add_definitions (-DCLIENT_RES_DIR="${CLIENT_DATA_DIR}/qt/client-res")
endif ()

if (APPLE)
    target_link_libraries (${PROJECT_NAME} ${LIBS} dcpp extra ${ZLIB_LIBRARIES})
elseif (HAIKU)
    target_link_libraries (${PROJECT_NAME} ${LIBS} extra dcpp be ${ZLIB_LIBRARIES})
else (APPLE)
    target_link_libraries (${PROJECT_NAME} ${LIBS} extra dcpp ${ZLIB_LIBRARIES})
endif (APPLE)

if (WIN32 OR APPLE OR HAIKU)
    set_property (TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME "${PROJECT_NAME_GLOBAL}")
else ()
    set_property (TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME "${PROJECT_NAME}")
endif ()

install (TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${BINDIR}
    BUNDLE DESTINATION ${BUNDLEDIR})

if (APPLE)
    set (plugin_dest_dir ${PROJECT_NAME_GLOBAL}.app/Contents/MacOS)
    set (qtconf_dest_dir ${PROJECT_NAME_GLOBAL}.app/Contents/Resources)
    set (APPS "\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME_GLOBAL}.app")
    set (DIRS "\${QT_LIBRARY_DIRS}" "\${PC_UPNP_LIBRARY_DIRS}" "\${CMAKE_INSTALL_PREFIX}/${LIBDIR}")
    set (CPACK_PACKAGE_ICON ${MACOSICON})
    set (CPACK_BUNDLE_ICON ${MACOSICON})
    set (CPACK_BUNDLE_NAME ${PROJECT_NAME_GLOBAL})
    if (CMAKE_OSX_DEPLOYMENT_TARGET)
        string (REGEX REPLACE "^(.*-.*)-g.*$" "\\1-macOS${CMAKE_OSX_DEPLOYMENT_TARGET}" FILE_NAME_VERSION "${APP_VERSION}")
        set (CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME_GLOBAL}-${FILE_NAME_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
    else (CMAKE_OSX_DEPLOYMENT_TARGET)
        set (CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME_GLOBAL}-${APP_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
    endif (CMAKE_OSX_DEPLOYMENT_TARGET)
    set_target_properties (${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/Info.plist.in)
    set (MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.eiskaltdcpp")
    set (MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_NAME_GLOBAL} ${APP_VERSION})
    set (MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME_GLOBAL})
    set (MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION})
    set (MACOSX_BUNDLE_BUNDLE_VERSION ${APP_VERSION})
    set (MACOSX_BUNDLE_COPYRIGHT "Copyright 2009-2021 EiskaltDC++ developers")
endif (APPLE)

if (USE_JS)
    install (DIRECTORY qtscripts/ DESTINATION ${CLIENT_SCRIPTS_DEST}
             PATTERN "*.sh"
             PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
             PATTERN "*.php"
             PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif (USE_JS)

install (FILES ${TRANSLATIONS_BINARY} DESTINATION ${CLIENT_TRANSLATIONS_DEST})

if (UNIX AND NOT APPLE AND NOT HAIKU)
    install (DIRECTORY icons/appl icons/user DESTINATION "${CLIENT_DATA_DIR}/qt/icons")
    install (FILES ${PROJECT_BINARY_DIR}/default.rcc DESTINATION "${CLIENT_DATA_DIR}/qt/client-res")
else ()
    install (DIRECTORY icons/appl icons/user DESTINATION "${CLIENT_DATA_DIR}/icons")
    install (FILES ${PROJECT_BINARY_DIR}/default.rcc DESTINATION "${CLIENT_DATA_DIR}/client-res")
endif ()

if (UNIX AND NOT APPLE AND NOT HAIKU)
    install (FILES ${PROJECT_NAME}.desktop DESTINATION ${SHARE_DIR}/applications)
endif ()

if (INSTALL_METAINFO)
    install (FILES ${PROJECT_NAME}.appdata.xml DESTINATION ${SHARE_DIR}/metainfo)
endif ()

if (APPLE)
    if (${_qt5Core_install_prefix} STREQUAL "")
    message(WARNING "Warning! \"_qt5Core_install_prefix\" is not set, macdeployqt found in PATH will be used")
    set (MACDEPLOYQT "macdeployqt")
    else (${_qt5Core_install_prefix} STREQUAL "")
    set (MACDEPLOYQT "${_qt5Core_install_prefix}/bin/macdeployqt")
    endif (${_qt5Core_install_prefix} STREQUAL "")
    # Help macdeployqt find Homebrew-linked libs (DuckDB, aspell, …).
    if (DuckDB_LIBRARY)
        get_filename_component (_MAC_LIBPATH "${DuckDB_LIBRARY}" DIRECTORY)
    else (DuckDB_LIBRARY)
        set (_MAC_LIBPATH "/opt/homebrew/lib")
    endif (DuckDB_LIBRARY)
    install (CODE "
                execute_process(COMMAND ${MACDEPLOYQT} ${PROJECT_NAME_GLOBAL}.app -verbose=1
                    -libpath=${_MAC_LIBPATH}
                    WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX})")

    # Explicitly embed DuckDB (macdeployqt may miss non-Qt dylibs). Self-contained .app.
    if (USE_QT_SQLITE AND DuckDB_LIBRARY)
        get_filename_component (_DUCKDB_REAL "${DuckDB_LIBRARY}" REALPATH)
        get_filename_component (_DUCKDB_NAME "${_DUCKDB_REAL}" NAME)
        install (CODE "
            set (BUNDLE_APP_NAME \"${PROJECT_NAME_GLOBAL}\")
            set (BUNDLE_DYLIB_SRC \"${DuckDB_LIBRARY}\")
            set (BUNDLE_DYLIB_REAL \"${_DUCKDB_REAL}\")
            set (BUNDLE_DYLIB_NAME \"${_DUCKDB_NAME}\")
            # Homebrew opt/ symlink and Cellar path both appear in otool -L.
            set (BUNDLE_DYLIB_EXTRA_OLDS \"/opt/homebrew/opt/duckdb/lib/${_DUCKDB_NAME};/usr/local/opt/duckdb/lib/${_DUCKDB_NAME}\")
            include (\"${PROJECT_SOURCE_DIR}/../macos/BundleHomebrewDylib.cmake\")
            ")
        # Build-tree .app: same embed so local runs do not need Homebrew at runtime.
        # install_name_tool -change/-add_rpath fail if the entry is missing/already set.
        # Build-tree .app: same embed so local runs do not need Homebrew at runtime.
        add_custom_command (TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                "-DBUNDLE_APP_NAME=${PROJECT_NAME_GLOBAL}"
                "-DBUNDLE_DYLIB_SRC=${DuckDB_LIBRARY}"
                "-DBUNDLE_DYLIB_REAL=${_DUCKDB_REAL}"
                "-DBUNDLE_DYLIB_NAME=${_DUCKDB_NAME}"
                "-DBUNDLE_DYLIB_EXTRA_OLDS=/opt/homebrew/opt/duckdb/lib/${_DUCKDB_NAME};/usr/local/opt/duckdb/lib/${_DUCKDB_NAME}"
                "-DCMAKE_INSTALL_PREFIX=$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/.."
                -P "${PROJECT_SOURCE_DIR}/../macos/BundleHomebrewDylib.cmake"
            VERBATIM)
    endif (USE_QT_SQLITE AND DuckDB_LIBRARY)

    # Embed libmediainfo + libzen (own-list media scan).
    # Use compatibility install names (libmediainfo.0.dylib), not the versioned REALPATH basename.
    if (USE_MEDIAINFO AND MediaInfo_LIBRARY AND Zen_LIBRARY)
        get_filename_component (_MI_REAL "${MediaInfo_LIBRARY}" REALPATH)
        get_filename_component (_ZEN_REAL "${Zen_LIBRARY}" REALPATH)
        set (_MI_NAME "libmediainfo.0.dylib")
        set (_ZEN_NAME "libzen.0.dylib")
        add_custom_command (TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                "-DBUNDLE_APP_NAME=${PROJECT_NAME_GLOBAL}"
                "-DBUNDLE_DYLIB_SRC=${MediaInfo_LIBRARY}"
                "-DBUNDLE_DYLIB_REAL=${_MI_REAL}"
                "-DBUNDLE_DYLIB_NAME=${_MI_NAME}"
                "-DBUNDLE_DYLIB_EXTRA_OLDS=/opt/homebrew/opt/libmediainfo/lib/${_MI_NAME};/usr/local/opt/libmediainfo/lib/${_MI_NAME}"
                "-DCMAKE_INSTALL_PREFIX=$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/.."
                -P "${PROJECT_SOURCE_DIR}/../macos/BundleHomebrewDylib.cmake"
            COMMAND ${CMAKE_COMMAND}
                "-DBUNDLE_APP_NAME=${PROJECT_NAME_GLOBAL}"
                "-DBUNDLE_DYLIB_SRC=${Zen_LIBRARY}"
                "-DBUNDLE_DYLIB_REAL=${_ZEN_REAL}"
                "-DBUNDLE_DYLIB_NAME=${_ZEN_NAME}"
                "-DBUNDLE_DYLIB_EXTRA_OLDS=/opt/homebrew/opt/libzen/lib/${_ZEN_NAME};/usr/local/opt/libzen/lib/${_ZEN_NAME}"
                "-DCMAKE_INSTALL_PREFIX=$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/.."
                -P "${PROJECT_SOURCE_DIR}/../macos/BundleHomebrewDylib.cmake"
            COMMAND bash -c "install_name_tool -change '/opt/homebrew/opt/libzen/lib/libzen.0.dylib' '@executable_path/../Frameworks/${_ZEN_NAME}' '$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/${_MI_NAME}' 2>/dev/null || true"
            COMMAND bash -c "install_name_tool -change '/usr/local/opt/libzen/lib/libzen.0.dylib' '@executable_path/../Frameworks/${_ZEN_NAME}' '$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/${_MI_NAME}' 2>/dev/null || true"
            COMMAND bash -c "codesign --remove-signature '$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/${_MI_NAME}' >/dev/null 2>&1 || true"
            COMMAND bash -c "codesign -s - --force --deep '$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>' >/dev/null 2>&1"
            VERBATIM)
    endif (USE_MEDIAINFO AND MediaInfo_LIBRARY AND Zen_LIBRARY)

    # data/ installs before this bundle copy; re-install emoticons so a stale
    # build-tree Resources/emoticons/*.xml cannot clobber the source map.
    if (WITH_EMOTICONS)
        install (DIRECTORY ${CMAKE_SOURCE_DIR}/data/emoticons/
                 DESTINATION ${CLIENT_DATA_DIR}/emoticons)
    endif (WITH_EMOTICONS)

    set (CPACK_BINARY_DRAGNDROP ON)
endif (APPLE)

if (POLICY CMP0077)
    cmake_policy (SET CMP0077 NEW)
endif ()

include(CPack)
