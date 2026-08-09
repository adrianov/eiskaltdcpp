/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * Slash-command interpreter for hub chat and PM windows (/away, /pm, aliases, …).
 */

#pragma once

class HubFrame;
class QString;
class QWidget;

class HubChatCmd
{
public:
    explicit HubChatCmd(HubFrame *hub);

    /** Returns true when line was a recognized command (consumed). */
    bool run(QString line, QWidget *target);

private:
    void reply(QWidget *target, const QString &msg) const;
    void say(QWidget *target, const QString &msg, bool thirdPerson) const;

    HubFrame *hub;
};
