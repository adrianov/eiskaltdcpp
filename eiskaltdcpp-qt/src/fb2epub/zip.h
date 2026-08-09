// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// In-memory EPUB ZIP packing for FB2→EPUB conversion.
//
#pragma once

#include <QByteArray>
#include <QString>
#include <utility>
#include <vector>

namespace HomeCompa
{

/** Pack EPUB members; first entry must be uncompressed "mimetype". */
[[nodiscard]] QByteArray PackEpubMembers(const std::vector<std::pair<QString, QByteArray>>& members);

} // namespace HomeCompa
