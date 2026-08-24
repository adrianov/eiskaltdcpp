/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include <utility>
#include <vector>

/**
 * Compiled vocabulary of adult-video cues: age tags, porn-site and studio
 * brands, performer names, erotic-cinema classics, explicit act/anatomy words
 * (Latin, Cyrillic, Japanese, Korean) and JAV label codes.
 *
 * Matching is layered: a case-sensitive release-brand pass (BLACKED.22.01.07,
 * VIXEN RAW), a first-char-indexed literal gate that rejects innocent rows
 * cheaply, one precompiled master regex over the lowercased haystack, then
 * mainstream-title vetoes («Sex Pistols» is a band, «Секс в большом городе»
 * a series). Constructed once, then read-only — safe to share across threads.
 */
class AdultCueSet
{
public:
    static const AdultCueSet &instance();

    /** True when the backslash-joined name/path carries any adult cue. */
    bool matches(const QString &hay) const;

private:
    AdultCueSet();

    using Range = std::pair<int, int>;
    bool mayMatch(const QString &lowered) const;
    void buildTriggers();

    QStringList m_substrings;
    QStringList m_bounded;
    QStringList m_phrases;
    QStringList m_exclusions;
    QStringList m_javLabels;
    QRegularExpression m_master;
    QRegularExpression m_brand;

    // Literal-gate state: every cue contains one of these literals, so a row
    // containing none cannot match m_master. Grouped by first UTF-16 unit.
    std::vector<QString> m_literals;
    std::vector<Range> m_table = std::vector<Range>(65536);
};
