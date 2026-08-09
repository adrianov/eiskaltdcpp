/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QApplication>
#include <QEvent>
#include <QTimer>
#include <QSessionManager>
#include "WulforSettings.h"
#include "MainWindow.h"
#include "qtsingleapp/qtsinglecoreapplication.h"
#include "dcpp/ProcessExit.h"
#include <objc/objc.h>
#include <objc/message.h>

bool dockClickHandler(id self,SEL _cmd,...);

/** Drop dock/UI so macOS does not beachball other apps during teardown. */
inline void resignMacAppForExit()
{
    typedef objc_object* (*object_type)(struct objc_object *self, SEL _cmd);
    object_type objc_msgSendObject = (object_type)objc_msgSend;

    objc_object* cls = (objc_object*)objc_getClass("NSApplication");
    objc_object* appInst = objc_msgSendObject(cls, sel_registerName("sharedApplication"));
    if (!appInst)
        return;

    typedef void (*hide_t)(objc_object*, SEL, objc_object*);
    ((hide_t)objc_msgSend)(appInst, sel_registerName("hide:"), nullptr);

    // NSApplicationActivationPolicyProhibited == 2
    typedef long (*policy_t)(objc_object*, SEL, long);
    ((policy_t)objc_msgSend)(appInst, sel_registerName("setActivationPolicy:"), 2L);
}

class EiskaltEventFilter: public QObject{
    Q_OBJECT

signals:
    void clickedOnDock();

public:
    EiskaltEventFilter(): counter(0), has_activity(true), prevAppState(Qt::ApplicationHidden) {
        timer.setInterval(60000);

        connect(&timer, SIGNAL(timeout()), this, SLOT(tick()));

        timer.start();
    }

    virtual ~EiskaltEventFilter() {}

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event){
        switch (event->type()){
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::Wheel:
        {
            has_activity = true;
            counter = 0;

            break;
        }
        case QEvent::ApplicationStateChange:
        {
            // https://stackoverflow.com/questions/15143369/qt-on-os-x-how-to-detect-clicking-the-app-dock-icon/46488514#46488514
            QApplicationStateChangeEvent *ev = static_cast<QApplicationStateChangeEvent*>(event);
            if (prevAppState == Qt::ApplicationActive && ev->applicationState() == Qt::ApplicationActive) {
                emit clickedOnDock();
            }
            prevAppState = ev->applicationState();
        }
        default:
        {
            has_activity = false;

            break;
        }
        }

        return QObject::eventFilter(obj, event);
    }

private Q_SLOTS:
    void tick(){
        if (!has_activity)
            ++counter;

        if (WBGET(WB_APP_AUTOAWAY_BY_TIMER)){
            int mins = WIGET(WI_APP_AUTOAWAY_INTERVAL);

            if (!mins)
                return;

            int mins_done = (counter*timer.interval()/1000)/60;

            if (mins <= mins_done)
                dcpp::Util::setAway(true);
        }
        else if (has_activity && !dcpp::Util::getManualAway())
            dcpp::Util::setAway(false);
    }

private:
    QTimer timer;
    int counter;
    bool has_activity;
    Qt::ApplicationState prevAppState;
};

class EiskaltApp: public QtSingleCoreApplication{
    Q_OBJECT

signals:
    void clickedOnDock();

public:
    EiskaltApp(int &argc, char *argv[], const QString& uniqKey): QtSingleCoreApplication(argc, argv, uniqKey)
    {
        installEventFilter(&ef);
        installMacHandlers();

        connect(&ef, SIGNAL(clickedOnDock()), this, SIGNAL(clickedOnDock()));
    }

    void commitData(QSessionManager& manager){
        // Save UI state, then release immediately. Heavy socket/DuckDB joins run
        // after app.exec() — not under the session manager (that freezes logout).
        dcpp::noteAppExiting();
        if (MainWindow::getInstance()){
            MainWindow::getInstance()->beginExit();
            MainWindow::getInstance()->close();
        }
        resignMacAppForExit();
        manager.release();
    }

    void saveState(QSessionManager &){ /** Do nothing */ }

private:
    EiskaltEventFilter ef;

    void installMacHandlers(){
        typedef objc_object* (*object_type)(struct objc_object *self, SEL _cmd);
        object_type objc_msgSendObject = (object_type)objc_msgSend;

        objc_object* cls = (objc_object*)objc_getClass("NSApplication");
        SEL sharedApplication = sel_registerName("sharedApplication");
        objc_object* appInst = objc_msgSendObject(cls, sharedApplication);

        if (appInst){
            objc_object* delegate = objc_msgSendObject(appInst,  sel_registerName("delegate"));
            objc_object* delClass = objc_msgSendObject(delegate, sel_registerName("class"));
            bool test = class_addMethod((objc_class*)delClass,
                                        sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:"),
                                        (IMP)dockClickHandler,"B@:");
            if (!test) {
                // failed to register handler...
            }
        }
    }
};
