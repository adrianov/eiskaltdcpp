// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include <algorithm>

#include "Fb2EpubImages.h"

#include "Fb2EpubNav.h"
#include "Fb2EpubText.h"
#include "xml/XmlAttributes.h"

namespace HomeCompa::Util
{

QString InlineImageFileName(const QString& id, const QString& mime)
{
	QString stem = id;
	for (auto& ch : stem)
	{
		if (!ch.isLetterOrNumber() && ch != '-' && ch != '_')
			ch = '_';
	}
	if (stem.isEmpty())
		stem = QStringLiteral("image");
	return QString("images/%1.%2").arg(stem, CoverExtension(mime.isEmpty() ? QStringLiteral("image/jpeg") : mime));
}

void ResolveBodyImagePlaceholders(
	QString&                              bodyHtml,
	std::vector<Fb2EmbeddedImage>&        images,
	const std::vector<QString>&           bodyImageIds,
	const QMap<QString, QString>&          imageAlts,
	const QMap<QString, QPair<QByteArray, QString>>& binaries,
	const QString&                        coverId
)
{
	for (const auto& id : bodyImageIds)
	{
		const auto marker = QString("{{FB2IMG:%1}}").arg(id);
		if (id.isEmpty() || id == coverId)
		{
			bodyHtml.replace(marker, QString {});
			continue;
		}

		const auto it = binaries.find(id);
		if (it == binaries.end())
		{
			bodyHtml.replace(marker, QString {});
			continue;
		}

		const auto fileName = InlineImageFileName(id, it->second);
		const bool exists = std::any_of(images.begin(), images.end(), [&](const Fb2EmbeddedImage& item) {
			return item.fileName == fileName;
		});
		if (!exists)
			images.push_back(Fb2EmbeddedImage { fileName, it->first, it->second });
		const auto alt = EscapeHtmlText(imageAlts.value(id));
		bodyHtml.replace(
			marker,
			QString("<p class=\"image\"><img src=\"%1\" alt=\"%2\" /></p>\n").arg(EscapeHtmlText(fileName), alt)
		);
	}
}

} // namespace HomeCompa::Util
