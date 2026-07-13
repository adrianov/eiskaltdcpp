cmake_minimum_required (VERSION 3.10)
project (dcpp)

aux_source_directory(${PROJECT_SOURCE_DIR} dcpp_srcs)
file (GLOB dcpp_hdrs ${PROJECT_SOURCE_DIR}/*.h)

# Explicit modules (also from aux_source_directory). Keeps splits visible; appends if glob ever removed.
set(DCPP_EXTRA_SOURCES
    DirectoryListingLoad.cpp
    DirectoryListingLoader.cpp
    DirectoryListingDir.cpp
    IncomingPortCheck.cpp
    SSLSocketHandshake.cpp
    CryptoManagerSsl.cpp
    CryptoManagerCert.cpp
    CryptoManagerCertGen.cpp
    ClientConnect.cpp
    ClientReconnect.cpp
    ClientManagerConnect.cpp
    ClientManagerConnectHub.cpp
    ClientManagerHubGuard.cpp
    ClientLocalIp.cpp
    ClientManagerUserLookup.cpp
    ClientSearch.cpp
    HubSearchDenied.cpp
    ListCache.cpp
    ListCacheStore.cpp
    ExpectedMap.cpp
    QueueAutoSearch.cpp
    QueueItemSegment.cpp
    SearchQuery.cpp
    SearchManagerUdp.cpp
    SearchManagerResponse.cpp
    QueueManagerIndex.cpp
    QueueManagerList.cpp
    QueueManagerListMatch.cpp
    QueueManagerLoad.cpp
    QueueManagerPut.cpp
    QueueManagerPutFinish.cpp
    QueueManagerSave.cpp
    QueueManagerSource.cpp
    QueueManagerUserQueue.cpp
    QueueManagerUserQueueNext.cpp
    NaturalCompare.cpp
    QueueManagerWorkers.cpp
    QueueManagerRecheck.cpp
    QueueManagerPartial.cpp
    QueueManagerSfv.cpp
    QueueManagerTimer.cpp
    QueueManagerGet.cpp
    QueueManagerRemove.cpp
    QueueManagerAdd.cpp
    QueueManagerListeners.cpp
    QueueManagerTarget.cpp
    NmdcHubLine.cpp
    NmdcHubLineProfile.cpp
    NmdcHubLineHub.cpp
    NmdcHubLineLists.cpp
    NmdcHubConnect.cpp
    AdcHubConnect.cpp
    AdcHubChat.cpp
    AdcHubStatus.cpp
    AdcHubSearch.cpp
    AdcHubBloom.cpp
    AdcHubInfo.cpp
    AdcHubUsers.cpp
    AdcHubInf.cpp
    AdcHubEvents.cpp
    NmdcHubSend.cpp
    NmdcHubUsers.cpp
    NmdcHubUserParse.cpp
    NmdcHubEscape.cpp
    PeerConnectLog.cpp
    PeerConnectFilter.cpp
    HubReconnectFilter.cpp
    PeerConnectTls.cpp
    DownloadManagerSlots.cpp
    DownloadManagerSlotFail.cpp
    DownloadManagerTimer.cpp
    UploadManagerTimer.cpp
    UploadManagerWaiting.cpp
    UploadManagerMinute.cpp
    UploadManagerTransfer.cpp
    UploadManagerPrepare.cpp
    UploadManagerPartial.cpp
    UploadManagerGrant.cpp
    UploadManagerAutoSlot.cpp
    UploadRequestGuard.cpp
    DownloadManagerTransfer.cpp
    ConnectionManagerQueue.cpp
    ConnectionManagerTimer.cpp
    ConnectionManagerGetDownload.cpp
    ConnectionManagerListen.cpp
    ConnectionManagerFail.cpp
    ConnectionManagerNmdcHandshake.cpp
    ConnectionManagerNmdcMyNick.cpp
    ConnectionManagerAssoc.cpp
    ConnectionManagerAdcHandshake.cpp
    ConnectionManagerConnectOut.cpp
    ConnectionManagerLifecycle.cpp
    PeerConnectLogFail.cpp
    LogManagerTrim.cpp
    ProcessExit.cpp
    SocketSocks.cpp
    SocketResolve.cpp
    UserConnectionLine.cpp
    UserConnectionChunk.cpp
    UserConnectionSpeed.cpp
    UtilIp.cpp
    UtilDisk.cpp
    UtilUrl.cpp
    QueueManagerDisk.cpp
    SettingsManagerDefaults.cpp
    SettingsManagerHublist.cpp
    SettingsManagerLoad.cpp
    SettingsManagerSave.cpp
    SettingsManagerSearch.cpp
    SettingsManagerAccess.cpp
    ScriptManagerLua.cpp
    ScriptManagerLuaQuery.cpp
    ScriptManagerEval.cpp
    ShareManagerFile.cpp
    ShareManagerRemoveDir.cpp
    FinishedManagerComplete.cpp
    FinishedManagerMap.cpp
)
foreach(_src ${DCPP_EXTRA_SOURCES})
    set(_path ${PROJECT_SOURCE_DIR}/${_src})
    if(NOT EXISTS ${_path})
        message(FATAL_ERROR "dcpp source missing: ${_src}")
    elseif(NOT "${_path}" IN_LIST dcpp_srcs)
        list(APPEND dcpp_srcs ${_path})
    endif()
endforeach()

list (REMOVE_ITEM dcpp_srcs ${PROJECT_SOURCE_DIR}/FileReader.cpp)
list (REMOVE_ITEM dcpp_hdrs ${PROJECT_SOURCE_DIR}/FileReader.h)

set (DCPP_SRCS ${dcpp_srcs} PARENT_SCOPE)
set (DCPP_HDRS ${dcpp_hdrs} PARENT_SCOPE)

if (NOT LUA_SCRIPT)
    list (REMOVE_ITEM dcpp_srcs
        ${PROJECT_SOURCE_DIR}/ScriptManager.cpp
        ${PROJECT_SOURCE_DIR}/ScriptManagerLua.cpp
        ${PROJECT_SOURCE_DIR}/ScriptManagerLuaQuery.cpp
        ${PROJECT_SOURCE_DIR}/ScriptManagerEval.cpp)
endif (NOT LUA_SCRIPT)

include_directories (${PROJECT_BINARY_DIR}
                     ${BZIP2_INCLUDE_DIR}
                     ${ZLIB_INCLUDE_DIR}
                     ${OPENSSL_INCLUDE_DIR}
                     ${GETTEXT_INCLUDE_DIR}
                     ${LUA_INCLUDE_DIR}
                     ${IDN2_INCLUDE_DIR})

if (WIN32)
    set (LINK STATIC)
    set (WIN32_LIBS "unistring" "ws2_32")
else (WIN32)
    if (HAIKU)
        set (LINK STATIC)
        set (PTHREADS "root")
        set (HAIKU_LIBS "network" "textencoding")
    else (HAIKU)
        set (PTHREADS "pthread")
        if (APPLE)
            set (LINK STATIC)
        elseif (BUILD_STATIC)
            set (LINK STATIC)
        endif(APPLE)
    endif(HAIKU)
endif (WIN32)

if (NOT LINK)
    set (LINK SHARED)
endif (NOT LINK)

if (APPLE)
    if (ICONV_LIBRARIES)
        set (APPLE_LIBS "")
    else ()
        set (APPLE_LIBS "iconv")
    endif ()
endif (APPLE)

set (UPNP "extra")

if (APPLE)
    set (DCPP_EXTRA_LINK "")
else ()
    set (DCPP_EXTRA_LINK ${UPNP})
endif ()

add_library (dcpp ${LINK} ${dcpp_srcs})

if (WITH_DHT)
    set (DHT_LIB "dht")
endif (WITH_DHT)

if (GETTEXT_FOUND)
    # Using GNU gettext does not guarantee that GNU libintl will be used. This
    # assumption breaks the build when using musl libc (which does implement
    # libintl, but does so without using internal GNU symbols).
    check_cxx_source_compiles("
        extern \"C\" int _nl_msg_cat_cntr;
        int main(void) {
            ++_nl_msg_cat_cntr;
            return 0;
        }"
        HAVE_NL_MSG_CAT_CNTR)
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/Util.h
                 PROPERTY COMPILE_DEFINITIONS HAVE_NL_MSG_CAT_CNTR APPEND)
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/Util.cpp
                 PROPERTY COMPILE_DEFINITIONS HAVE_NL_MSG_CAT_CNTR APPEND)
    if (NOT HAVE_NL_MSG_CAT_CNTR)
        message (STATUS "Build without GNU libintl library")
    endif ()
endif (GETTEXT_FOUND)

if (PERL_REGEX)
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/ADLSearch.cpp
                 PROPERTY COMPILE_DEFINITIONS USE_PCRE )
    set(PCRE ${PCRE2_LIBRARIES})
endif (PERL_REGEX)

if (USE_IDN2)
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/UtilUrl.cpp
                 PROPERTY COMPILE_DEFINITIONS USE_IDN2 APPEND)
endif(USE_IDN2)

if (XATTR_FOUND)
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/HashManager.h
                 PROPERTY COMPILE_DEFINITIONS USE_XATTR APPEND)
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/HashManager.cpp
                 PROPERTY COMPILE_DEFINITIONS USE_XATTR APPEND)
endif (XATTR_FOUND)

if (USE_MINIUPNP)
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/DCPlusPlus.cpp
                        ${PROJECT_SOURCE_DIR}/MappingManager.cpp
                 PROPERTY COMPILE_DEFINITIONS USE_MINIUPNP )
endif()

if (ICONV_SECOND_ARGUMENT_IS_CONST)
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/Text.cpp
                 PROPERTY COMPILE_DEFINITIONS ICONV_CONST=const)
endif (ICONV_SECOND_ARGUMENT_IS_CONST)

if (FORCE_XDG)
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/Util.cpp
                 PROPERTY COMPILE_DEFINITIONS FORCE_XDG APPEND)
endif (FORCE_XDG)

if (HAVE_IFADDRS_H)
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/Util.cpp
                        ${PROJECT_SOURCE_DIR}/UtilIp.cpp
                 PROPERTY COMPILE_DEFINITIONS HAVE_IFADDRS_H APPEND)
endif (HAVE_IFADDRS_H)

if (WIN32 OR CYGWIN)
    set_property(TARGET dcpp PROPERTY COMPILE_FLAGS)
else ()
    set_property(TARGET dcpp PROPERTY COMPILE_FLAGS -fPIC)
endif ()

target_compile_definitions (dcpp PRIVATE OPENSSL_API_COMPAT=0x10100000L)

target_link_libraries (dcpp
                       ${DHT_LIB}
                       ${PTHREADS}
                       ${IDN2_LIBRARIES}
                       ${BZIP2_LIBRARIES}
                       ${ZLIB_LIBRARIES}
                       ${OPENSSL_LIBRARIES}
                       ${GETTEXT_LIBRARIES}
                       ${ICONV_LIBRARIES}
                       ${WIN32_LIBS}
                       ${APPLE_LIBS}
                       ${HAIKU_LIBS}
                       ${LUA_LIBRARIES}
                       ${DCPP_EXTRA_LINK}
                       ${PCRE}
                       ${XATTR_LIBRARIES})
set_target_properties(dcpp PROPERTIES
                      VERSION ${SO_VERSION}
                      SOVERSION ${SO_VERSION}
                      OUTPUT_NAME "eiskaltdcpp")

if (APPLE)
    set (LIBDIR ${PROJECT_NAME_GLOBAL}.app/Contents/Frameworks)
    set_target_properties(dcpp PROPERTIES BUILD_WITH_INSTALL_RPATH 1
                          INSTALL_NAME_DIR "@executable_path/../Frameworks")
endif(APPLE)

if (LINK STREQUAL SHARED)
    if (WITH_DEV_FILES)
        install (TARGETS dcpp LIBRARY DESTINATION ${LIBDIR})
    else (WITH_DEV_FILES)
        install (TARGETS dcpp LIBRARY DESTINATION ${LIBDIR} NAMELINK_SKIP)
    endif (WITH_DEV_FILES)
    message (STATUS "Build libeiskaltdcpp as shared library")
elseif (LINK STREQUAL STATIC AND WITH_DEV_FILES)
    install (TARGETS dcpp LIBRARY DESTINATION ${LIBDIR})
    message (STATUS "Build libeiskaltdcpp as static library")
endif (LINK STREQUAL SHARED)

if (WITH_DEV_FILES)
    if (NOT EISKALTDCPP_INCLUDE_DIR)
        set (EISKALTDCPP_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include/eiskaltdcpp")
    endif (NOT EISKALTDCPP_INCLUDE_DIR)
    message (STATUS "Headers of libeiskaltdcpp will be installed to: ${EISKALTDCPP_INCLUDE_DIR}")
    install (FILES ${dcpp_hdrs} DESTINATION ${EISKALTDCPP_INCLUDE_DIR}/dcpp/)

    set (prefix ${CMAKE_INSTALL_PREFIX})
    set (libdir ${LIBDIR})
    set (includedir ${EISKALTDCPP_INCLUDE_DIR})
    configure_file (${CMAKE_SOURCE_DIR}/eiskaltdcpp.pc.in ${CMAKE_BINARY_DIR}/eiskaltdcpp.pc @ONLY)
    install (FILES ${CMAKE_BINARY_DIR}/eiskaltdcpp.pc
             DESTINATION ${CMAKE_INSTALL_PREFIX}/${LIBDIR}/pkgconfig)
endif (WITH_DEV_FILES)

foreach(LANG ${LANGUAGES})
    install(FILES ${dcpp_BINARY_DIR}/${LANG}_libeiskaltdcpp.mo
            DESTINATION ${LOCALE_DIR}/${LANG}/LC_MESSAGES
            RENAME libeiskaltdcpp.mo)
endforeach(LANG ${LANGUAGES})
