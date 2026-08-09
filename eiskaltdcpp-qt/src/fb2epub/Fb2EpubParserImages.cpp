// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include <algorithm>

#include "Fb2EpubImages.h"

#include "Fb2EpubNav.h"
#include "Fb2EpubParserImpl.h"
#include "Fb2EpubText.h"
#include "xml/XmlAttributes.h"

namespace HomeCompa {
namespace Util
{

namespace
{

constexpr int kMaxCoverPriorChars = 160;

bool PngSize(const QByteArray& data, int& width, int& height)
{
	if (data.size() < 24 || !data.startsWith("\x89PNG\r\n\x1a\n"))
		return false;
	const auto* p = reinterpret_cast<const uchar*>(data.constData());
	width  = (p[16] << 24) | (p[17] << 16) | (p[18] << 8) | p[19];
	height = (p[20] << 24) | (p[21] << 16) | (p[22] << 8) | p[23];
	return width > 0 && height > 0;
}

bool GifSize(const QByteArray& data, int& width, int& height)
{
	if (data.size() < 10 || !data.startsWith("GIF8"))
		return false;
	const auto* p = reinterpret_cast<const uchar*>(data.constData());
	width  = p[6] | (p[7] << 8);
	height = p[8] | (p[9] << 8);
	return width > 0 && height > 0;
}

bool IsJpegSofMarker(uchar marker)
{
	return marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC;
}

int JpegSegmentLength(const uchar* p, int n, int i)
{
	if (i + 4 > n)
		return -1;
	const int seglen = (p[i + 2] << 8) | p[i + 3];
	if (seglen < 2 || i + 2 + seglen > n)
		return -1;
	return seglen;
}

bool ReadJpegSofSize(const uchar* p, int n, int& width, int& height)
{
	for (int i = 2; i + 8 < n;)
	{
		if (p[i] != 0xFF)
		{
			++i;
			continue;
		}
		const uchar marker = p[i + 1];
		if (marker == 0xD9 || marker == 0xDA)
			return false;
		if (marker == 0xD8 || (marker >= 0xD0 && marker <= 0xD7))
		{
			i += 2;
			continue;
		}
		const int seglen = JpegSegmentLength(p, n, i);
		if (seglen < 0)
			return false;
		if (IsJpegSofMarker(marker))
		{
			height = (p[i + 5] << 8) | p[i + 6];
			width  = (p[i + 7] << 8) | p[i + 8];
			return width > 0 && height > 0;
		}
		i += 2 + seglen;
	}
	return false;
}

bool JpegSize(const QByteArray& data, int& width, int& height)
{
	if (data.size() < 4 || !data.startsWith("\xFF\xD8\xFF"))
		return false;
	return ReadJpegSofSize(reinterpret_cast<const uchar*>(data.constData()), data.size(), width, height);
}

bool ImageSizeFromData(const QByteArray& data, int& width, int& height)
{
	return JpegSize(data, width, height) || PngSize(data, width, height) || GifSize(data, width, height);
}

QString PlainTextBefore(const QString& html, int end)
{
	QString out;
	out.reserve(end);
	bool inTag = false;
	for (int i = 0; i < end; ++i)
	{
		const QChar c = html.at(i);
		if (c == u'<')
		{
			inTag = true;
			continue;
		}
		if (c == u'>')
		{
			inTag = false;
			continue;
		}
		if (!inTag)
			out.append(c);
	}
	return CollapseWhitespace(out);
}

bool PriorLooksLikeCoverLeadIn(const QString& bodyHtml, const QString& imageId)
{
	const auto marker = QString("{{FB2IMG:%1}}").arg(imageId);
	const int  at     = bodyHtml.indexOf(marker);
	if (at < 0)
		return false;
	const auto prior = bodyHtml.left(at);
	if (prior.contains(QLatin1String("<h1")))
		return false;
	return PlainTextBefore(bodyHtml, at).size() <= kMaxCoverPriorChars;
}

} // namespace

bool LooksLikeCoverImage(const QByteArray& data)
{
	int width = 0;
	int height = 0;
	if (!ImageSizeFromData(data, width, height))
		return data.size() >= 30000;

	if (width < 180 || height < 180 || std::max(width, height) < 300)
		return false;
	const double ratio = double(width) / double(height);
	return ratio >= 0.4 && ratio <= 2.5;
}

void Fb2Parser::EnsureCoverFromBody()
{
	// FB2s without <coverpage>: promote first body image only when it looks like a cover.
	if (!coverData.isEmpty() || !coverId.isEmpty() || bodyImageIds.empty())
		return;

	const auto& id = bodyImageIds.front();
	if (!PriorLooksLikeCoverLeadIn(bodyBuffer, id))
		return;

	const auto it = binaries.constFind(id);
	if (it == binaries.constEnd() || it->first.isEmpty() || !LooksLikeCoverImage(it->first))
		return;

	coverId   = id;
	coverData = it->first;
	coverMime = it->second;
}

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

} // namespace Util
} // namespace HomeCompa
