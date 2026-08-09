// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubText.h"

#include <QDateTime>

namespace HomeCompa::Util
{

namespace
{

bool IsWhitespace(QChar c)
{
	return c.isSpace();
}

bool HasLeadingWhitespace(const QString& text)
{
	return !text.isEmpty() && IsWhitespace(text.front());
}

} // namespace

void AppendSpaceIfNeeded(QString& text)
{
	if (text.isEmpty())
		return;

	if (!text.back().isSpace())
		text.append(' ');
}

void AppendSeparatorIfNeeded(QString& text)
{
	if (text.isEmpty())
		return;

	if (!text.back().isSpace())
		text.append(QStringLiteral(" — "));
}

QString CollapseWhitespace(const QString& text)
{
	if (text.isEmpty())
		return {};

	QString out;
	out.reserve(text.size());
	auto seenSpace = false;
	for (const auto c : text)
	{
		if (IsWhitespace(c))
		{
			seenSpace = true;
			continue;
		}
		if (seenSpace && !out.isEmpty())
			out.append(' ');
		seenSpace = false;
		out.append(c);
	}
	return out;
}

namespace
{

bool IsWordChar(QChar c)
{
	return c.isLetter() || c.isNumber();
}

bool ScriptsDiffer(QChar a, QChar b)
{
	if (!IsWordChar(a) || !IsWordChar(b))
		return false;

	const auto aScript = a.script();
	const auto bScript = b.script();
	return aScript != bScript && aScript != QChar::Script_Common && bScript != QChar::Script_Common;
}

bool NeedsSpaceBefore(const QString& text, int index)
{
	const auto prev = text.at(index - 1);
	const auto next = text.at(index);

	if (IsWordChar(prev) && IsWordChar(next))
		return ScriptsDiffer(prev, next);

	return IsWordChar(prev) && next == u'[';
}

} // namespace

bool NeedsSpaceAfterPunctuation(QChar /*wordBeforePunct*/, QChar /*punct*/, QChar /*nextWordChar*/)
{
	return false;
}

QString SplitGluedWords(const QString& text)
{
	if (text.size() < 2)
		return text;

	QString out;
	out.reserve(text.size() + 8);
	out.append(text.front());
	for (int i = 1; i < text.size(); ++i)
	{
		if (NeedsSpaceBefore(text, i))
			out.append(u' ');
		out.append(text.at(i));
	}
	return out;
}

void AppendCollapsedText(QString& dest, const QString& text)
{
	const auto collapsed = CollapseWhitespace(text);
	if (collapsed.isEmpty())
		return;

	if (HasLeadingWhitespace(text) && !dest.isEmpty() && !IsWhitespace(dest.back()))
		dest.append(' ');

	dest.append(collapsed);
}

QString SanitizeXmlText(const QString& text)
{
	QString result;
	result.reserve(text.size());
	for (const auto ch : text)
	{
		if (ch.isSurrogate())
			continue;

		const auto code = ch.unicode();
		if (code == 0x9 || code == 0xA || code == 0xD || (code >= 0x20 && code <= 0xD7FF) || (code >= 0xE000 && code <= 0xFFFD))
			result.append(ch);
	}
	return result;
}

QString EscapeXmlText(const QString& text)
{
	QString escaped = SanitizeXmlText(text);
	escaped.replace('&', "&amp;");
	escaped.replace('<', "&lt;");
	escaped.replace('>', "&gt;");
	escaped.replace('"', "&quot;");
	return escaped;
}

QString EscapeHtmlText(const QString& text)
{
	QString escaped = SanitizeXmlText(text);
	escaped.replace('&', "&amp;");
	escaped.replace('<', "&lt;");
	escaped.replace('>', "&gt;");
	escaped.replace('"', "&quot;");
	escaped.replace('\'', "&#39;");
	return escaped;
}

QString EpubTimestampUtc()
{
	return QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyy-MM-dd'T'HH:mm:ss'Z'"));
}

QString EpubContentStyles()
{
	return QStringLiteral(
		"<style type=\"text/css\">\n"
		"@namespace epub \"http://www.idpf.org/2007/ops\";\n"
		"sup { font-size: 0.75em; line-height: normal; }\n"
		"sub { font-size: 0.75em; line-height: normal; }\n"
		"a[epub|type~=\"noteref\"] { text-decoration: none; }\n"
		"p.image { text-align: center; margin: 1em 0; }\n"
		"p.image img { max-width: 100%; height: auto; }\n"
		"blockquote { margin: 1em 2em; }\n"
		"p.text-author { text-align: right; font-style: italic; margin-top: 0.5em; }\n"
		"aside.annotation { margin: 1em 0; padding: 0.5em 1em; border-left: 3px solid #ccc; }\n"
		"table { border-collapse: collapse; margin: 1em auto; }\n"
		"th, td { border: 1px solid #ccc; padding: 0.25em 0.5em; }\n"
		"p.subtitle { font-style: italic; text-align: center; }\n"
		"section.book-annotation { margin: 1em 0; padding: 0.5em 1em; font-style: italic; }\n"
		"</style>");
}

QString PrependBookAnnotation(const QString& bodyHtml, const QString& annotation)
{
	const auto text = annotation.trimmed();
	if (text.isEmpty())
		return bodyHtml;

	QString section = QStringLiteral("<section epub:type=\"abstract\" class=\"book-annotation\">\n");
	for (const auto& paragraph : text.split(QStringLiteral("\n\n"), Qt::SkipEmptyParts))
	{
		const auto trimmed = paragraph.trimmed();
		if (!trimmed.isEmpty())
			section.append(QStringLiteral("<p>%1</p>\n").arg(EscapeHtmlText(trimmed)));
	}
	section.append(QStringLiteral("</section>\n"));
	return section + bodyHtml;
}

QString CoverMimeFromData(const QByteArray& data)
{
	if (data.startsWith("\x89PNG"))
		return QStringLiteral("image/png");
	if (data.startsWith("\xFF\xD8\xFF"))
		return QStringLiteral("image/jpeg");
	if (data.startsWith("GIF8"))
		return QStringLiteral("image/gif");
	if (data.size() >= 12 && data.mid(8, 4) == "WEBP")
		return QStringLiteral("image/webp");
	return QStringLiteral("image/jpeg");
}

} // namespace HomeCompa::Util
