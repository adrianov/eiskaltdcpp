// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#pragma once

#include "Fb2EpubMeta.h"

namespace HomeCompa::Util
{

void AppendFb2Genre(Fb2Metadata& metadata, const QString& value);
void SplitFb2Keywords(Fb2Metadata& metadata, const QString& value);

} // namespace HomeCompa::Util
