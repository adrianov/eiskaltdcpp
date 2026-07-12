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
#include "dcpp/SettingsManager.h"
#include "dcpp/Socket.h"

#include <QMessageBox>

using namespace dcpp;

void SettingsConnection::ok() {
    bool active = !radioButton_PASSIVE->isChecked();
    SettingsManager *SM = SettingsManager::getInstance();

    int old_mode = SETTING(INCOMING_CONNECTIONS);
    SM->set(SettingsManager::AUTO_DETECT_CONNECTION, checkBox_AUTO_DETECT_CONNECTION->isChecked());
    if(active) {
        if(radioButton_ACTIVE->isChecked())
            SM->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_DIRECT);
        else if(radioButton_PORT->isChecked())
            SM->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_NAT);
#if (defined USE_MINIUPNP)
        else
            SM->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_UPNP);
#endif
        SM->set(SettingsManager::TCP_PORT, spinBox_TCP->value());
        SM->set(SettingsManager::UDP_PORT, spinBox_UDP->value());

        if(spinBox_TLS->value() != SETTING(TCP_PORT))
            SM->set(SettingsManager::TLS_PORT, spinBox_TLS->value());
        else
            SM->set(SettingsManager::TLS_PORT, spinBox_TLS->value() + 1);

        SM->set(SettingsManager::EXTERNAL_IP, lineEdit_WANIP->text().toStdString());
        QString bind_ip = lineEdit_BIND_ADDRESS->text();
        if(validateIp(bind_ip))
            SM->set(SettingsManager::BIND_ADDRESS, lineEdit_BIND_ADDRESS->text().toStdString());
        SM->set(SettingsManager::NO_IP_OVERRIDE, checkBox_DONTOVERRIDE->checkState() == Qt::Checked);
    } else {
        SM->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_PASSIVE);
    }

    bool use_socks = !radioButton_DC->isChecked();
    int type = SETTING(OUTGOING_CONNECTIONS);

    SM->set(SettingsManager::BIND_IFACE, radioButton_BIND_IFACE->isChecked());
    SM->set(SettingsManager::BIND_IFACE_NAME, _tq(comboBox_IFACES->currentText()));

    if(use_socks) {
        QString ip = lineEdit_SIP->text();

        if(!validateIp(ip)) {
            showMsg(tr("No valid SOCKS5 server IP found!"), nullptr);
            return;
        }

        int port = lineEdit_SPORT->text().toInt();

        SM->set(SettingsManager::SOCKS_SERVER, lineEdit_SIP->text().toStdString());
        SM->set(SettingsManager::SOCKS_USER, lineEdit_SUSR->text().toStdString());
        SM->set(SettingsManager::SOCKS_PASSWORD, lineEdit_SPSWD->text().toStdString());
        SM->set(SettingsManager::SOCKS_RESOLVE, checkBox_RESOLVE->checkState() == Qt::Checked);
        SM->set(SettingsManager::OUTGOING_CONNECTIONS, SettingsManager::OUTGOING_SOCKS5);

        if(port > 0 && port <= 65535)
            SM->set(SettingsManager::SOCKS_PORT, port);
    } else {
        SM->set(SettingsManager::OUTGOING_CONNECTIONS, SettingsManager::OUTGOING_DIRECT);
    }

    if(SETTING(OUTGOING_CONNECTIONS) != type)
        Socket::socksUpdated();

    SM->set(SettingsManager::THROTTLE_ENABLE, checkBox_THROTTLE_ENABLE->isChecked());
    SM->set(SettingsManager::TIME_DEPENDENT_THROTTLE, checkBox_TIME_DEPENDENT_THROTTLE->isChecked());
    SM->set(SettingsManager::MAX_DOWNLOAD_SPEED_MAIN, spinBox_DOWN_LIMIT_NORMAL->value());
    SM->set(SettingsManager::MAX_UPLOAD_SPEED_MAIN, spinBox_UP_LIMIT_NORMAL->value());
    SM->set(SettingsManager::MAX_DOWNLOAD_SPEED_ALTERNATE, spinBox_DOWN_LIMIT_TIME->value());
    SM->set(SettingsManager::MAX_UPLOAD_SPEED_ALTERNATE, spinBox_UP_LIMIT_TIME->value());
    SM->set(SettingsManager::BANDWIDTH_LIMIT_START, spinBox_BANDWIDTH_LIMIT_START->value());
    SM->set(SettingsManager::BANDWIDTH_LIMIT_END, spinBox_BANDWIDTH_LIMIT_END->value());
    SM->set(SettingsManager::SLOTS_ALTERNATE_LIMITING, spinBox_ALTERNATE_SLOTS->value());
    SM->set(SettingsManager::RECONNECT_DELAY, spinBox_RECONNECT_DELAY->value());
    SM->set(SettingsManager::IP_TOS_VALUE, comboBox_TOS->itemData(comboBox_TOS->currentIndex()).toInt());
    SM->set(SettingsManager::DYNDNS_SERVER, lineEdit_DYNDNS_SERVER->text().toStdString());
    SM->set(SettingsManager::DYNDNS_ENABLE, checkBox_DYNDNS->isChecked());

#ifdef WITH_DHT
    SM->set(SettingsManager::USE_DHT, groupBox_DHT->isChecked());
    if(spinBox_DHT->value() != SETTING(UDP_PORT))
        SM->set(SettingsManager::DHT_PORT, spinBox_DHT->value());
    else
        SM->set(SettingsManager::DHT_PORT, spinBox_DHT->value() + 1);

    if(!(old_dht < 1024) && (SETTING(DHT_PORT) < 1024))
        showMsg(tr("Program need root privileges to open ports less than 1024"), nullptr);
#endif
    SM->set(SettingsManager::ALLOW_UNTRUSTED_CLIENTS, checkBox_UNTRUSTED_CLIENTS->isChecked());
    SM->set(SettingsManager::ALLOW_UNTRUSTED_HUBS, checkBox_UNTRUSTED_HUBS->isChecked());

    SM->set(SettingsManager::USE_TLS, (comboBox_TLS->currentIndex() == 1) || (comboBox_TLS->currentIndex() == 2));
    SM->set(SettingsManager::REQUIRE_TLS, (comboBox_TLS->currentIndex() == 2));

    if(old_mode != SETTING(INCOMING_CONNECTIONS) || old_tcp != (SETTING(TCP_PORT))
       || old_udp != (SETTING(UDP_PORT)) || old_tls != (SETTING(TLS_PORT)))
    {
        if(!(old_tcp < 1024 || old_tls < 1024 || old_udp < 1024) &&
           (SETTING(TCP_PORT) < 1024 || SETTING(UDP_PORT) < 1024 || SETTING(TLS_PORT) < 1024))
            showMsg(tr("Program need root privileges to open ports less than 1024"), nullptr);

        MainWindow::getInstance()->startSocket(true);
    }
}
