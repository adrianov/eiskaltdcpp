// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//

#pragma once

#include <QByteArray>
#include <QString>

#include "Fb2EpubParser.h"

#include <utility>
#include <vector>

namespace HomeCompa::Util
{

using EpubMember = std::pair<QString, QByteArray>;

[[nodiscard]] std::vector<EpubMember> BuildEpubMembers(const ParsedFb2& parsed, const QString& title);
[[nodiscard]] QByteArray              BuildEpubBytes(const ParsedFb2& parsed, const QString& title);
[[nodiscard]] bool                    WriteEpubBytesToFile(const QByteArray& epubBytes, const QString& epubPath);

} // namespace HomeCompa::Util
