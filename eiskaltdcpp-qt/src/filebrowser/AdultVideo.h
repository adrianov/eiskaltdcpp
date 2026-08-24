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

class QString;

/**
 * Adult-video cue matcher over a file name plus its share path.
 *
 * Covers age tags ([18+], +18), porn-site and studio brands, well-known
 * performer names, erotic-cinema classics, JAV label codes (SSIS-123, FC2-PPV),
 * and explicit act/anatomy words in Latin, Cyrillic, Japanese and Korean.
 * Tokens that would false-positive as substrings («анализ» vs «анал»,
 * NUD4700 LED drivers vs nude) are matched on Unicode-aware word boundaries;
 * ambiguous lowercase brand spellings (Vixen, Deeper) require release-style
 * uppercase spelling with a date or RAW suffix.
 */
namespace AdultVideo {

bool matches(const QString &name, const QString &path);

} // namespace AdultVideo
