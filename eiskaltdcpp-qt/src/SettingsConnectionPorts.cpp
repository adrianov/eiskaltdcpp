/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SettingsConnection.h"
#include "MainWindow.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/ClientManager.h"
#include "dcpp/IncomingPortCheck.h"
#include "dcpp/Util.h"

#ifdef WITH_DHT
#include "dht/DHT.h"
#endif

#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>

using namespace dcpp;

namespace {

QLabel* makePortDot(QWidget* parent) {
    auto* lab = new QLabel(parent);
    lab->setFixedSize(12, 12);
    lab->setAlignment(Qt::AlignCenter);
    return lab;
}

void paintPortDot(QLabel* lab, IncomingPortCheck::Result result, const QString& tip) {
    const char* color = "#9e9e9e";
    switch(result) {
    case IncomingPortCheck::Result::Open:
        color = "#2ecc40";
        break;
    case IncomingPortCheck::Result::Closed:
        color = "#e74c3c";
        break;
    default:
        break;
    }
    lab->setStyleSheet(QStringLiteral(
        "QLabel { background-color: %1; border-radius: 6px; border: 1px solid rgba(0,0,0,40%); }")
        .arg(QLatin1String(color)));
    lab->setToolTip(tip);
}

IncomingPortCheck::Result resultForKind(const string& kind, int spinPort, bool passive) {
    if(passive)
        return IncomingPortCheck::Result::Closed;

    auto* chk = IncomingPortCheck::getInstance();
    const string checked = chk->getPort(kind);
    if(checked.empty() || Util::toInt(checked) != spinPort)
        return IncomingPortCheck::Result::Unknown;
    return chk->getResult(kind);
}

QString tipForResult(IncomingPortCheck::Result r, bool passive, const QString& kind) {
    if(passive)
        return QObject::tr("Passive mode — incoming connections are disabled.");
    switch(r) {
    case IncomingPortCheck::Result::Open:
        return QObject::tr("%1 port is reachable from the internet.").arg(kind);
    case IncomingPortCheck::Result::Closed:
        return QObject::tr("%1 port is blocked or not reachable from the internet.").arg(kind);
    default:
        return QObject::tr("%1 port status is unknown. Use “Check incoming ports”.").arg(kind);
    }
}

} // namespace

void SettingsConnection::initPortDots() {
    auto* grid = findChild<QGridLayout*>(QStringLiteral("gridLayout_4"));
    if(grid) {
        QWidget* host = spinBox_TCP->parentWidget();
        portDotTcp = makePortDot(host);
        portDotUdp = makePortDot(host);
        portDotTls = makePortDot(host);
        grid->addWidget(portDotTcp, 0, 2);
        grid->addWidget(portDotUdp, 1, 2);
        grid->addWidget(portDotTls, 2, 2);
    }

#ifdef WITH_DHT
    auto* dhtGrid = findChild<QGridLayout*>(QStringLiteral("gridLayout_13"));
    if(dhtGrid) {
        portDotDht = makePortDot(spinBox_DHT->parentWidget());
        dhtGrid->addWidget(portDotDht, 0, 2);
    }
#endif

    connect(spinBox_TCP, SIGNAL(valueChanged(int)), this, SLOT(slotRefreshPortDots()));
    connect(spinBox_UDP, SIGNAL(valueChanged(int)), this, SLOT(slotRefreshPortDots()));
    connect(spinBox_TLS, SIGNAL(valueChanged(int)), this, SLOT(slotRefreshPortDots()));
#ifdef WITH_DHT
    connect(spinBox_DHT, SIGNAL(valueChanged(int)), this, SLOT(slotRefreshPortDots()));
    connect(groupBox_DHT, SIGNAL(toggled(bool)), this, SLOT(slotRefreshPortDots()));
#endif
    connect(radioButton_PASSIVE, SIGNAL(toggled(bool)), this, SLOT(slotRefreshPortDots()));

    IncomingPortCheck::getInstance()->addListener(this);
    slotRefreshPortDots();
}

void SettingsConnection::shutdownPortDots() {
    IncomingPortCheck::getInstance()->removeListener(this);
}

void SettingsConnection::slotRefreshPortDots() {
    const bool passive = radioButton_PASSIVE->isChecked();

    if(portDotTcp) {
        auto r = resultForKind("TCP", spinBox_TCP->value(), passive);
        paintPortDot(portDotTcp, r, tipForResult(r, passive, tr("TCP")));
    }
    if(portDotUdp) {
        auto r = resultForKind("UDP", spinBox_UDP->value(), passive);
        QString tip;
        if(passive) {
            tip = tipForResult(r, true, tr("UDP"));
        } else if(r == IncomingPortCheck::Result::Open) {
            tip = tr("UDP port is active — datagrams have been received.");
        } else {
            tip = tr("UDP port status is unknown until search replies arrive.");
        }
        paintPortDot(portDotUdp, r, tip);
    }
    if(portDotTls) {
        auto r = resultForKind("TLS", spinBox_TLS->value(), passive);
        paintPortDot(portDotTls, r, tipForResult(r, passive, tr("TLS")));
    }

#ifdef WITH_DHT
    if(portDotDht) {
        IncomingPortCheck::Result r = IncomingPortCheck::Result::Unknown;
        QString tip = tr("DHT port status is unknown.");
        if(!groupBox_DHT->isChecked()) {
            tip = tr("DHT is disabled.");
        } else if(passive || !ClientManager::getInstance()->isActive()) {
            r = IncomingPortCheck::Result::Closed;
            tip = tipForResult(r, true, tr("DHT"));
        } else if(dht::DHT::getInstance()) {
            const string bound = dht::DHT::getInstance()->getPort();
            if(bound.empty()) {
                tip = tr("DHT is not listening yet.");
            } else if(Util::toInt(bound) != spinBox_DHT->value()) {
                tip = tr("DHT port changed — save settings to apply.");
            } else if(dht::DHT::getInstance()->isFirewalled()) {
                r = IncomingPortCheck::Result::Closed;
                tip = tr("DHT UDP port appears firewalled (blocked).");
            } else {
                r = IncomingPortCheck::Result::Open;
                tip = tr("DHT UDP port appears open (approved).");
            }
        }
        paintPortDot(portDotDht, r, tip);
    }
#endif
}

void SettingsConnection::slotPortCheckResult(QString kind, int result) {
    Q_UNUSED(kind);
    Q_UNUSED(result);
    slotRefreshPortDots();
}

void SettingsConnection::slotTestPorts() {
    if(radioButton_PASSIVE->isChecked()) {
        showMsg(tr("Incoming port check requires active mode."), pushButton_TEST_PORTS);
        return;
    }

    MainWindow::getInstance()->startSocket(true);
    IncomingPortCheck::getInstance()->start();
    slotRefreshPortDots();
    showMsg(tr("Port check started. Status dots update when results arrive."), nullptr);
}

void SettingsConnection::on(IncomingPortCheckListener::PortResult, const string& kind,
                            const string&, int result) noexcept {
    emit corePortResult(_q(kind), result);
}

void SettingsConnection::on(IncomingPortCheckListener::Finished) noexcept {
    emit corePortResult(QString(), static_cast<int>(IncomingPortCheck::Result::Unknown));
}
