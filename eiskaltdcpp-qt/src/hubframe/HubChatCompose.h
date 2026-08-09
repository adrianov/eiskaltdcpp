/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * Hub chat input composition: outgoing message history (Up/Down) and smile panel.
 */

#pragma once

#include <QStringList>

class HubFrame;
class QObject;

class HubChatCompose
{
public:
    explicit HubChatCompose(HubFrame *hub);

    void remember(const QString &msg);
    void nextMsg();
    void prevMsg();

    void toggleSmiles();
    void smileClicked(QObject *sender);
    void smileThemeMenu();
    void rebuildPanel();
    void clearPanel();

private:
    void insertSmile(QString smiley);

    HubFrame *hub;
    QStringList messages;
    int index = 0;
    bool unsent = false;
};
