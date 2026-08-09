// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubNav.h"

#include "Fb2EpubText.h"

namespace HomeCompa::Util
{

QString CoverExtension(const QString& mime)
{
	const auto lower = mime.toLower();
	if (lower == "image/jpeg" || lower == "image/jpg")
		return "jpg";
	if (lower == "image/png")
		return "png";
	if (lower == "image/gif")
		return "gif";
	if (lower == "image/webp")
		return "webp";
	return "jpg";
}

QString NavItemsHtml(const ParsedFb2& parsed, bool includeCover, const QString& fallbackTitle)
{
	QString items;
	if (includeCover)
		items.append("      <li><a href=\"cover.xhtml\">Cover</a></li>\n");

	if (parsed.tocItems.empty())
	{
		const auto title = EscapeHtmlText(fallbackTitle.isEmpty() ? QStringLiteral("Book") : fallbackTitle);
		items.append(QString("      <li><a href=\"content.xhtml\">%1</a></li>\n").arg(title));
		return items;
	}

	for (size_t i = 0; i < parsed.tocItems.size();)
	{
		const auto& item = parsed.tocItems[i];
		const auto  title = EscapeHtmlText(item.title.isEmpty() ? QStringLiteral("Section") : item.title);
		items.append(QString("      <li><a href=\"content.xhtml#%1\">%2</a>").arg(item.id, title));
		++i;

		QString subItems;
		while (i < parsed.tocItems.size() && parsed.tocItems[i].level == 2)
		{
			const auto& sub = parsed.tocItems[i++];
			const auto  subTitle = EscapeHtmlText(sub.title.isEmpty() ? QStringLiteral("Section") : sub.title);
			subItems.append(QString("          <li><a href=\"content.xhtml#%1\">%2</a></li>\n").arg(sub.id, subTitle));
		}

		if (subItems.isEmpty())
			items.append("</li>\n");
		else
			items.append(QString("\n        <ol>\n%1        </ol>\n      </li>\n").arg(subItems));
	}
	return items;
}

} // namespace HomeCompa::Util
