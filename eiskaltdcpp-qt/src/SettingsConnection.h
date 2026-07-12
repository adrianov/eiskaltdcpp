/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QWidget>
#include <QIntValidator>
#include <QEvent>
#include <QKeyEvent>

#include "ui_UISettingsConnection.h"
#include "SettingsInterface.h"

#include "dcpp/stdinc.h"
#include "dcpp/IncomingPortCheckListener.h"

class QLabel;

class SettingsConnection :
        public QWidget,
        private Ui::UISettingsConnection,
        private dcpp::IncomingPortCheckListener
{
    Q_OBJECT
public:
    SettingsConnection(QWidget* = nullptr);
    ~SettingsConnection() override;

public slots:
    void ok();

signals:
    void corePortResult(QString kind, int result);

protected:
    bool eventFilter(QObject*, QEvent*) override;

private slots:
    void slotToggleIncomming();
    void slotToggleOutgoing();
    void slotTestPorts();
    void slotRefreshPortDots();
    void slotPortCheckResult(QString kind, int result);

private:
    void init();
    void initPortDots();
    void shutdownPortDots();

    bool validateIp(QString&);
    void showMsg(QString, QWidget* = nullptr);

    void on(dcpp::IncomingPortCheckListener::PortResult, const std::string& kind,
            const std::string& port, int result) noexcept override;
    void on(dcpp::IncomingPortCheckListener::Finished) noexcept override;

    bool dirty;

    int old_tcp, old_udp, old_tls
#ifdef WITH_DHT
        , old_dht
#endif
        ;

    QLabel* portDotTcp = nullptr;
    QLabel* portDotUdp = nullptr;
    QLabel* portDotTls = nullptr;
#ifdef WITH_DHT
    QLabel* portDotDht = nullptr;
#endif
};
