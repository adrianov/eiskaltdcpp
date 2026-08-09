// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>
#include <vector>

#include "xml/XmlAttributes.h"

namespace HomeCompa::Util
{

struct Fb2EmbeddedImage
{
	QString    fileName;
	QByteArray data;
	QString    mime;
};

inline QString RefIdFromHref(QString href)
{
	const auto index = href.indexOf('#');
	if (index >= 0)
		return index + 1 < href.size() ? href.mid(index + 1).trimmed() : QString {};
	return href.trimmed();
}

inline QString RefIdFromAttr(const XmlAttributes& attributes)
{
	for (size_t i = 0, count = attributes.GetCount(); i < count; ++i)
	{
		if (!attributes.GetName(i).endsWith(":href"))
			continue;

		if (const auto id = RefIdFromHref(attributes.GetValue(i)); !id.isEmpty())
			return id;
	}
	return {};
}

QString InlineImageFileName(const QString& id, const QString& mime);
void    ResolveBodyImagePlaceholders(
	QString&                              bodyHtml,
	std::vector<Fb2EmbeddedImage>&        images,
	const std::vector<QString>&           bodyImageIds,
	const QMap<QString, QString>&          imageAlts,
	const QMap<QString, QPair<QByteArray, QString>>& binaries,
	const QString&                        coverId
);

} // namespace HomeCompa::Util
