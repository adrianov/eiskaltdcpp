// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#pragma once

#include <QByteArray>
#include <QString>
#include <vector>

#include "Fb2EpubImages.h"
#include "Fb2EpubMeta.h"

class QIODevice;

namespace HomeCompa::Util
{

struct Fb2TocItem
{
	QString id;
	QString title;
	int     level { 1 };
};

struct ParsedFb2
{
	QString                   title;
	QString                   author;
	QString                   language;
	Fb2Metadata               metadata;
	QString                   bodyHtml;
	QByteArray                coverData;
	QString                   coverMime;
	std::vector<Fb2EmbeddedImage> images;
	std::vector<Fb2TocItem>   tocItems;
};

bool ParseFb2(QIODevice& stream, ParsedFb2& result);

} // namespace HomeCompa::Util
