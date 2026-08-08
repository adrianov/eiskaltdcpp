/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QThread>
#include <functional>

/** Runs one function on a worker thread, then finishes (used by share browser load/search). */
class AsyncRunner : public QThread {
public:
    explicit AsyncRunner(QObject *parent = nullptr);
    ~AsyncRunner() override;

    void run() override;
    void setRunFunction(const std::function<void()> &f);

private:
    std::function<void()> runFunc;
};
