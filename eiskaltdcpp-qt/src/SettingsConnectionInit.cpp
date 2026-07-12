/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SettingsConnection.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/SettingsManager.h"

#include <QLineEdit>

#ifndef IPTOS_TOS_MASK
#define	IPTOS_TOS_MASK		0x1E
#endif
#ifndef IPTOS_TOS
#define	IPTOS_TOS(tos)		((tos) & IPTOS_TOS_MASK)
#endif
#ifndef IPTOS_LOWDELAY
#define	IPTOS_LOWDELAY		0x10
#endif
#ifndef IPTOS_THROUGHPUT
#define	IPTOS_THROUGHPUT	0x08
#endif
#ifndef IPTOS_RELIABILITY
#define	IPTOS_RELIABILITY	0x04
#endif
#ifndef IPTOS_LOWCOST
#define	IPTOS_LOWCOST		0x02
#endif
#ifndef IPTOS_MINCOST
#define	IPTOS_MINCOST		IPTOS_LOWCOST
#endif

using namespace dcpp;

void SettingsConnection::init() {
    lineEdit_WANIP->setText(QString::fromStdString(SETTING(EXTERNAL_IP)));
    lineEdit_BIND_ADDRESS->setText(QString::fromStdString(SETTING(BIND_ADDRESS)));

    spinBox_TCP->setValue(old_tcp = SETTING(TCP_PORT));
    spinBox_UDP->setValue(old_udp = SETTING(UDP_PORT));
    spinBox_TLS->setValue(old_tls = SETTING(TLS_PORT));
    checkBox_AUTO_DETECT_CONNECTION->setChecked(BOOLSETTING(AUTO_DETECT_CONNECTION));
    checkBox_THROTTLE_ENABLE->setChecked(BOOLSETTING(THROTTLE_ENABLE));
    checkBox_TIME_DEPENDENT_THROTTLE->setChecked(BOOLSETTING(TIME_DEPENDENT_THROTTLE));
    spinBox_DOWN_LIMIT_NORMAL->setValue(SETTING(MAX_DOWNLOAD_SPEED_MAIN));
    spinBox_UP_LIMIT_NORMAL->setValue(SETTING(MAX_UPLOAD_SPEED_MAIN));
    spinBox_DOWN_LIMIT_TIME->setValue(SETTING(MAX_DOWNLOAD_SPEED_ALTERNATE));
    spinBox_UP_LIMIT_TIME->setValue(SETTING(MAX_UPLOAD_SPEED_ALTERNATE));
    spinBox_BANDWIDTH_LIMIT_START->setValue(SETTING(BANDWIDTH_LIMIT_START));
    spinBox_BANDWIDTH_LIMIT_END->setValue(SETTING(BANDWIDTH_LIMIT_END));
    spinBox_ALTERNATE_SLOTS->setValue(SETTING(SLOTS_ALTERNATE_LIMITING));
    spinBox_RECONNECT_DELAY->setValue(SETTING(RECONNECT_DELAY));
    checkBox_DONTOVERRIDE->setCheckState(SETTING(NO_IP_OVERRIDE) ? Qt::Checked : Qt::Unchecked);
    checkBox_DYNDNS->setCheckState(BOOLSETTING(DYNDNS_ENABLE) ? Qt::Checked : Qt::Unchecked);
    lineEdit_DYNDNS_SERVER->setText(QString::fromStdString(SETTING(DYNDNS_SERVER)));

#ifdef WITH_DHT
    groupBox_DHT->setChecked(BOOLSETTING(USE_DHT));
    spinBox_DHT->setValue(old_dht = SETTING(DHT_PORT));
#else
    groupBox_DHT->hide();
#endif

    checkBox_UNTRUSTED_CLIENTS->setChecked(SETTING(ALLOW_UNTRUSTED_CLIENTS));
    checkBox_UNTRUSTED_HUBS->setChecked(SETTING(ALLOW_UNTRUSTED_HUBS));

    comboBox_TLS->setCurrentIndex(0);
    if(SETTING(USE_TLS))
        comboBox_TLS->setCurrentIndex(1);
    if(SETTING(REQUIRE_TLS))
        comboBox_TLS->setCurrentIndex(2);

    QStringList ifaces = WulforUtil::getInstance()->getLocalIfaces();
    if(!ifaces.isEmpty())
        comboBox_IFACES->addItems(ifaces);
    comboBox_IFACES->addItem("");

    if(SETTING(BIND_IFACE)) {
        radioButton_BIND_IFACE->toggle();
        if(ifaces.contains(_q(SETTING(BIND_IFACE_NAME))))
            comboBox_IFACES->setCurrentIndex(ifaces.indexOf(_q(SETTING(BIND_IFACE_NAME))));
        else
            comboBox_IFACES->setCurrentIndex(comboBox_IFACES->count() - 1);
    } else {
        radioButton_BIND_ADDR->toggle();
    }

    switch(SETTING(INCOMING_CONNECTIONS)) {
    case SettingsManager::INCOMING_DIRECT:
        radioButton_ACTIVE->setChecked(true);
        break;
    case SettingsManager::INCOMING_FIREWALL_NAT:
        radioButton_PORT->setChecked(true);
        break;
    case SettingsManager::INCOMING_FIREWALL_PASSIVE:
        radioButton_PASSIVE->setChecked(true);
        break;
#if (defined USE_MINIUPNP)
    case SettingsManager::INCOMING_FIREWALL_UPNP:
        radioButton_UPNP->setChecked(true);
        break;
#endif
    }
#if (!defined USE_MINIUPNP)
    radioButton_UPNP->setEnabled(false);
#endif

    lineEdit_SIP->setText(QString::fromStdString(SETTING(SOCKS_SERVER)));
    lineEdit_SUSR->setText(QString::fromStdString(SETTING(SOCKS_USER)));
    lineEdit_SPORT->setText(QString().setNum(SETTING(SOCKS_PORT)));
    lineEdit_SPSWD->setText(QString::fromStdString(SETTING(SOCKS_PASSWORD)));
    checkBox_RESOLVE->setCheckState(SETTING(SOCKS_RESOLVE) ? Qt::Checked : Qt::Unchecked);

    switch(SETTING(OUTGOING_CONNECTIONS)) {
    case SettingsManager::OUTGOING_DIRECT:
        radioButton_DC->toggle();
        break;
    case SettingsManager::OUTGOING_SOCKS5:
        radioButton_SOCKS->toggle();
        break;
    }

    comboBox_TOS->setItemData(0, -1);
    comboBox_TOS->setItemData(1, IPTOS_LOWDELAY);
    comboBox_TOS->setItemData(2, IPTOS_THROUGHPUT);
    comboBox_TOS->setItemData(3, IPTOS_RELIABILITY);
    comboBox_TOS->setItemData(4, IPTOS_MINCOST);

    for(int i = 0; i < comboBox_TOS->count(); i++) {
        if(comboBox_TOS->itemData(i).toInt() == SETTING(IP_TOS_VALUE)) {
            comboBox_TOS->setCurrentIndex(i);
            break;
        }
    }

    slotToggleIncomming();
    slotToggleOutgoing();

    connect(radioButton_ACTIVE, SIGNAL(toggled(bool)), this, SLOT(slotToggleIncomming()));
    connect(radioButton_PORT, SIGNAL(toggled(bool)), this, SLOT(slotToggleIncomming()));
    connect(radioButton_PASSIVE, SIGNAL(toggled(bool)), this, SLOT(slotToggleIncomming()));
#if (defined USE_MINIUPNP)
    connect(radioButton_UPNP, SIGNAL(toggled(bool)), this, SLOT(slotToggleIncomming()));
#endif
    connect(radioButton_DC, SIGNAL(toggled(bool)), this, SLOT(slotToggleOutgoing()));
    connect(radioButton_SOCKS, SIGNAL(toggled(bool)), this, SLOT(slotToggleOutgoing()));
    connect(pushButton_TEST_PORTS, SIGNAL(clicked()), this, SLOT(slotTestPorts()));
    connect(this, SIGNAL(corePortResult(QString,int)), this, SLOT(slotPortCheckResult(QString,int)),
            Qt::QueuedConnection);

    lineEdit_SIP->installEventFilter(this);
    lineEdit_SPORT->installEventFilter(this);
    lineEdit_SPSWD->installEventFilter(this);
    lineEdit_SUSR->installEventFilter(this);
    lineEdit_WANIP->installEventFilter(this);
    spinBox_TCP->installEventFilter(this);
    spinBox_UDP->installEventFilter(this);
    spinBox_TLS->installEventFilter(this);
    radioButton_ACTIVE->installEventFilter(this);
    radioButton_DC->installEventFilter(this);
    radioButton_PASSIVE->installEventFilter(this);
    radioButton_PORT->installEventFilter(this);
    radioButton_SOCKS->installEventFilter(this);
#if (defined USE_MINIUPNP)
    radioButton_UPNP->installEventFilter(this);
#endif
    checkBox_DONTOVERRIDE->installEventFilter(this);
    checkBox_RESOLVE->installEventFilter(this);

    initPortDots();
}
