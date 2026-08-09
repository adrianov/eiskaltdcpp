// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubMetaParse.h"

namespace HomeCompa::Util
{

void AppendFb2Genre(Fb2Metadata& metadata, const QString& value)
{
	const auto genre = value.trimmed();
	if (!genre.isEmpty() && !metadata.genres.contains(genre))
		metadata.genres.append(genre);
}

void SplitFb2Keywords(Fb2Metadata& metadata, const QString& value)
{
	for (const auto& part : value.split(',', Qt::SkipEmptyParts))
	{
		const auto keyword = part.trimmed();
		if (!keyword.isEmpty() && !metadata.keywords.contains(keyword))
			metadata.keywords.append(keyword);
	}
}

} // namespace HomeCompa::Util
