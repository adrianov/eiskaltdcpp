// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#pragma once

#include <QString>

#include "Fb2EpubParser.h"

namespace HomeCompa::Util
{

QString CoverExtension(const QString& mime);
QString NavItemsHtml(const ParsedFb2& parsed, bool includeCover, const QString& fallbackTitle);

} // namespace HomeCompa::Util
