/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * Slash-command interpreter for hub chat and PM windows (/away, /pm, aliases, …).
 */

#pragma once

class HubFrame;
class QString;
class QStringList;
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

    bool doAway(QWidget *target, QString &line, bool emptyParam) const;
    bool doAlias(QWidget *target, const QString &line) const;
    bool doKword(QWidget *target, const QStringList &list) const;
    bool doRatio(QWidget *target, const QString &param, bool emptyParam) const;
    bool doHelp(QWidget *target) const;
    bool doMe(QWidget *target, QString &line) const;
    bool doSh(QWidget *target, QString &line) const;
    bool doAliasExpand(QWidget *target, const QString &cmd);

    HubFrame *hub;
};
