/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "HashProgress.h"
#include "WulforUtil.h"

#include <QDir>
#include <QFontMetrics>

#include "dcpp/stdinc.h"
#include "dcpp/HashManager.h"
#include "dcpp/ShareManager.h"
#include "dcpp/TimerManager.h"

using namespace dcpp;

namespace {

QString elidePath(const QFontMetrics &metrics, int maxWidth, const QString &path)
{
    if (metrics.horizontalAdvance(path) <= maxWidth)
        return path;

    const QStringList parts = path.split(QDir::separator(), WULFOR_SKIP_EMPTY);
    if (parts.size() <= 1)
        return path;

    QString out;
    for (int i = parts.size() - 1; i >= 0; --i) {
        const QString next = parts.at(i) + (out.isEmpty() ? out : (QDir::separator() + out));
        if (metrics.horizontalAdvance(next) >= maxWidth) {
            out = QStringLiteral("..") + QDir::separator() + out;
            break;
        }
        out = next;
    }
    return out.isEmpty() ? parts.last() : out;
}

void setIdleRates(QLabel *status, QLabel *speed, QString &eta, size_t files, uint64_t bytes)
{
    status->setText(HashProgress::tr("-.-- files/h, %1 files left").arg(static_cast<uint32_t>(files)));
    speed->setText(HashProgress::tr("-.-- B/s, %1 left").arg(WulforUtil::formatBytes(bytes)));
    eta = HashProgress::tr("-:--:--");
}

void setActiveRates(QLabel *status, QLabel *speed, QProgressBar *progress, QString &eta,
                    uint64_t startBytes, size_t startFiles, uint64_t bytes, size_t files,
                    double diff)
{
    const double fileRate = ((double)(startFiles - files) * 60 * 60 * 1000) / diff;
    const double byteRate = ((double)(startBytes - bytes) * 1000) / diff;

    status->setText(HashProgress::tr("%1 files/h, %2 files left")
                        .arg(fileRate)
                        .arg(static_cast<uint32_t>(files)));
    speed->setText(HashProgress::tr("%1/s, %2 left, %3 shared")
                       .arg(WulforUtil::formatBytes(static_cast<int64_t>(byteRate)))
                       .arg(WulforUtil::formatBytes(bytes))
                       .arg(WulforUtil::formatBytes(ShareManager::getInstance()->getShareSize())));

    if (byteRate == 0.0) {
        eta = HashProgress::tr("-:--:--");
    } else {
        eta = _q(Text::toT(Util::formatSeconds(static_cast<int64_t>(bytes / byteRate))));
    }
    progress->setFormat(HashProgress::tr("%p% %1 left").arg(eta));
}

} // namespace

unsigned HashProgress::getHashStatus()
{
    ShareManager *SM = ShareManager::getInstance();
    HashManager *HM = HashManager::getInstance();
    if (SM->isRefreshing())
        return LISTUPDATE;
    if (HM->isHashingPaused())
        return (Util::getUpTime() < SETTING(HASHING_START_DELAY)) ? DELAYED : PAUSED;

    string path;
    uint64_t bytes = 0;
    size_t files = 0;
    HM->getStats(path, bytes, files);
    return (bytes || files) ? RUNNING : IDLE;
}

HashProgress::HashProgress(QWidget *parent)
    : QDialog(parent)
    , autoClose(false)
    , startBytes(0)
    , startFiles(0)
    , startTime(0)
{
    setupUi(this);
    setWindowModality(Qt::ApplicationModal);
    // Keep the process alive when this dialog is only hidden.
    setAttribute(Qt::WA_QuitOnClose, false);

    progressIndicator->hide();
    adjustSize();

    timer = new QTimer();
    timer->setInterval(250);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTick()));
    connect(pushButton_START, SIGNAL(clicked()), this, SLOT(slotStart()));
    connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(slotAutoClose(bool)));
    timer->start();
}

void HashProgress::resetProgress()
{
    startBytes = 0;
    startFiles = 0;
    startTime = 0;
}

HashProgress::~HashProgress()
{
    timer->stop();
    delete timer;
}

float HashProgress::getProgress()
{
    return static_cast<float>(progress->value()) / progress->maximum();
}

void HashProgress::timerTick()
{
    string path;
    uint64_t bytes = 0;
    size_t files = 0;
    const uint64_t tick = GET_TICK();

    stateButton();
    HashManager::getInstance()->getStats(path, bytes, files);

    if (ShareManager::getInstance()->isRefreshing()) {
        file->setText(tr("Refreshing file list"));
        return;
    }

    if (startTime == 0)
        startTime = tick;
    if (bytes > startBytes)
        startBytes = bytes;
    if (files > startFiles)
        startFiles = files;
    if (autoClose && !files) {
        accept();
        return;
    }

    const double diff = static_cast<double>(tick - startTime);
    const bool paused = HashManager::getInstance()->isHashingPaused();
    progress->setValue(startFiles == 0 || startBytes == 0
                           ? 0
                           : static_cast<int>((10000 * (startBytes - bytes)) / startBytes));

    QString eta;
    if (static_cast<uint64_t>(diff) == 0 || files == 0 || bytes == 0 || paused)
        setIdleRates(status, speed, eta, files, bytes);
    else
        setActiveRates(status, speed, progress, eta, startBytes, startFiles, bytes, files, diff);

    if (!files) {
        file->setText(tr("Done"));
        return;
    }

    const QString full = QString::fromStdString(path);
    file->setToolTip(full);
    file->setText(elidePath(QFontMetrics(font()), file->width() * 3 / 4, full));
}

void HashProgress::slotStart()
{
    ShareManager *SM = ShareManager::getInstance();
    HashManager *HM = HashManager::getInstance();
    switch (getHashStatus()) {
    case IDLE:
        SM->setDirty();
        SM->refresh(true);
        break;
    case LISTUPDATE:
    case RUNNING:
        HM->pauseHashing();
        break;
    case DELAYED:
    case PAUSED:
        HM->resumeHashing();
        break;
    }
    stateButton();
}

void HashProgress::slotAutoClose(bool b)
{
    autoClose = b;
    blockSignals(true);
    checkBox->setChecked(b);
    blockSignals(false);
}

void HashProgress::stateButton()
{
    switch (getHashStatus()) {
    case IDLE:
        pushButton_START->setText(tr("Start"));
        break;
    case LISTUPDATE:
    case RUNNING:
        pushButton_START->setText(tr("Pause"));
        break;
    case DELAYED:
    case PAUSED:
        pushButton_START->setText(tr("Resume"));
        break;
    }
}
