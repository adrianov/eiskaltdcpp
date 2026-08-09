// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2 path helpers for FB2→EPUB conversion.
//
#pragma once

#include "export/util.h"

#include <QString>
#include <QStringList>

namespace HomeCompa {
namespace Util
{

UTIL_EXPORT bool IsFb2Suffix(const QString& suffix);
UTIL_EXPORT bool IsFb2Path(const QString& path);
// CLI args after argv[0]: existing .fb2/.fbd paths (absolute), in order.
UTIL_EXPORT QStringList CollectFb2Args(const QStringList& args);
UTIL_EXPORT bool IsEpubSuffix(const QString& suffix);
UTIL_EXPORT bool IsEpubPath(const QString& path);
// INPX FileName without Ext (e.g. "249292") or a path with suffix.
UTIL_EXPORT bool IsExportableEpubSource(const QString& fileName);
UTIL_EXPORT QString ResolveArchiveBookFile(const QStringList& archiveFiles, const QString& bookFile);

} // namespace Util
} // namespace HomeCompa
