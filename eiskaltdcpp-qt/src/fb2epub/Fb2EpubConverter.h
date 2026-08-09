// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion, ported from FLibrary for eiskaltdcpp-qt.
//
#pragma once

#include "export/util.h"

#include <QByteArray>
#include <QString>

namespace HomeCompa {
namespace Util
{

struct EpubCover
{
    QByteArray data;
    QString    mime;
};

struct Fb2ToEpubOptions
{
    const EpubCover* coverOverride { nullptr };
};

UTIL_EXPORT bool ConvertFb2ToEpub(const QString& fb2Path, const QString& epubPath, const Fb2ToEpubOptions* options = nullptr);

UTIL_EXPORT bool ConvertFb2BytesToEpub(
    const QByteArray&       fb2Bytes,
    const QString&          fb2FileName,
    const QString&          epubPath,
    const Fb2ToEpubOptions* options = nullptr
);

UTIL_EXPORT QString EpubPathForFb2(const QString& fb2Path);

} // namespace Util
} // namespace HomeCompa
