// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubParserImpl.h"

#include "Fb2EpubText.h"

namespace HomeCompa::Util
{

namespace
{

QString TableCellOpen(const char* tag, const XmlAttributes& attributes)
{
	const auto align = attributes.GetAttribute("align").trimmed().toLower();
	if (align.isEmpty() || align == "left")
		return QString("<%1>").arg(tag);
	const auto css = align == "justify" ? QStringLiteral("justify") : align;
	return QString("<%1 style=\"text-align:%2;\">").arg(tag, css);
}

QString StyleOpenTag(const XmlAttributes& attributes)
{
	const auto name = attributes.GetAttribute("name").trimmed();
	if (name.compare("strong", Qt::CaseInsensitive) == 0 || name == "#b" || name == "#bold")
		return QStringLiteral("<strong>");
	if (name.compare("emphasis", Qt::CaseInsensitive) == 0 || name == "#i" || name == "#italic")
		return QStringLiteral("<em>");
	if (name.compare("code", Qt::CaseInsensitive) == 0)
		return QStringLiteral("<code>");
	if (name.compare("strikethrough", Qt::CaseInsensitive) == 0)
		return QStringLiteral("<s>");
	if (name.compare("underline", Qt::CaseInsensitive) == 0 || name == "not_supported_in_fb2_underline")
		return QStringLiteral("<u>");
	if (name.isEmpty())
		return QStringLiteral("<span>");
	return QString("<span class=\"fb2-%1\">").arg(EscapeHtmlText(name));
}

QString StyleCloseTag(const QString& openTag)
{
	if (openTag.startsWith(QStringLiteral("<strong")))
		return QStringLiteral("</strong>");
	if (openTag.startsWith(QStringLiteral("<em")))
		return QStringLiteral("</em>");
	if (openTag.startsWith(QStringLiteral("<code")))
		return QStringLiteral("</code>");
	if (openTag == QStringLiteral("<s>"))
		return QStringLiteral("</s>");
	if (openTag.startsWith(QStringLiteral("<u")))
		return QStringLiteral("</u>");
	return QStringLiteral("</span>");
}

} // namespace

bool Fb2Parser::InTextContent() const
{
	return inParagraph || inVerse || inTitle || inSubtitle || inEmphasis || inStrong || inLink || inCode || inStrike
	    || inSub || inSup || inStyle || inTextAuthor || inTableCell;
}

void Fb2Parser::AppendBlockHtml(const QString& html)
{
	if (!InContent())
		return;
	ActiveBuffer().append(html);
}

void Fb2Parser::OpenInlineTag(const QString& openTag, bool& flag, bool tight)
{
	flag            = true;
	inlineWordChars = 0;
	if (!tight)
		MarkInlineOpenBoundary();
	AppendSpaceBeforeInlineIfNeeded();
	ActiveBuffer().append(openTag);
	if (tight)
		needSpaceBeforeText = false;
}

void Fb2Parser::CloseInlineTag(const QString& closeTag, bool& flag, bool tight)
{
	flag = false;
	ActiveBuffer().append(closeTag);
	if (tight)
	{
		inlineWordChars     = 0;
		needSpaceBeforeText = false;
		afterTightInline    = true;
	}
	else
		MarkInlineCloseBoundary();
}

bool Fb2Parser::TryBodyExtraStart(const QString& name, const XmlAttributes& attributes)
{
	if (name.compare("cite", Qt::CaseInsensitive) == 0)
	{
		inCite = true;
		AppendBlockHtml("<blockquote>\n");
		return true;
	}
	if (name.compare("epigraph", Qt::CaseInsensitive) == 0)
	{
		inEpigraph = true;
		AppendBlockHtml("<blockquote epub:type=\"epigraph\">\n");
		return true;
	}
	if (name.compare("annotation", Qt::CaseInsensitive) == 0)
	{
		inAnnotation = true;
		AppendBlockHtml("<aside class=\"annotation\">\n");
		return true;
	}
	if (name.compare("text-author", Qt::CaseInsensitive) == 0)
	{
		inTextAuthor = true;
		AppendBlockHtml("<p class=\"text-author\">");
		return true;
	}
	if (name.compare("table", Qt::CaseInsensitive) == 0)
	{
		inTable = true;
		AppendBlockHtml("<table>\n");
		return true;
	}
	if (name.compare("tr", Qt::CaseInsensitive) == 0)
	{
		AppendBlockHtml("<tr>\n");
		return true;
	}
	if (name.compare("th", Qt::CaseInsensitive) == 0)
	{
		inTableCell = true;
		AppendBlockHtml(TableCellOpen("th", attributes));
		return true;
	}
	if (name.compare("td", Qt::CaseInsensitive) == 0)
	{
		inTableCell = true;
		AppendBlockHtml(TableCellOpen("td", attributes));
		return true;
	}
	if (name.compare("code", Qt::CaseInsensitive) == 0)
	{
		OpenInlineTag(QStringLiteral("<code>"), inCode, true);
		return true;
	}
	if (name.compare("strikethrough", Qt::CaseInsensitive) == 0)
	{
		OpenInlineTag(QStringLiteral("<s>"), inStrike);
		return true;
	}
	if (name.compare("sub", Qt::CaseInsensitive) == 0)
	{
		OpenInlineTag(QStringLiteral("<sub>"), inSub, true);
		return true;
	}
	if (name.compare("sup", Qt::CaseInsensitive) == 0)
	{
		OpenInlineTag(QStringLiteral("<sup>"), inSup, true);
		return true;
	}
	if (name.compare("style", Qt::CaseInsensitive) == 0)
	{
		styleOpenTag    = StyleOpenTag(attributes);
		styleCloseTag   = StyleCloseTag(styleOpenTag);
		inStyle         = true;
		inlineWordChars = 0;
		AppendSpaceBeforeInlineIfNeeded();
		ActiveBuffer().append(styleOpenTag);
		needSpaceBeforeText = false;
		return true;
	}
	return false;
}

bool Fb2Parser::TryBodyExtraEnd(const QString& name)
{
	if (name.compare("cite", Qt::CaseInsensitive) == 0)
	{
		inCite = false;
		AppendBlockHtml("</blockquote>\n");
		return true;
	}
	if (name.compare("epigraph", Qt::CaseInsensitive) == 0)
	{
		inEpigraph = false;
		AppendBlockHtml("</blockquote>\n");
		return true;
	}
	if (name.compare("annotation", Qt::CaseInsensitive) == 0)
	{
		inAnnotation = false;
		AppendBlockHtml("</aside>\n");
		return true;
	}
	if (name.compare("text-author", Qt::CaseInsensitive) == 0)
	{
		inTextAuthor = false;
		AppendBlockHtml("</p>\n");
		return true;
	}
	if (name.compare("table", Qt::CaseInsensitive) == 0)
	{
		inTable = false;
		AppendBlockHtml("</table>\n");
		return true;
	}
	if (name.compare("tr", Qt::CaseInsensitive) == 0)
	{
		AppendBlockHtml("</tr>\n");
		return true;
	}
	if (name.compare("th", Qt::CaseInsensitive) == 0)
	{
		inTableCell = false;
		AppendBlockHtml("</th>");
		return true;
	}
	if (name.compare("td", Qt::CaseInsensitive) == 0)
	{
		inTableCell = false;
		AppendBlockHtml("</td>");
		return true;
	}
	if (name.compare("code", Qt::CaseInsensitive) == 0)
	{
		CloseInlineTag(QStringLiteral("</code>"), inCode, true);
		return true;
	}
	if (name.compare("strikethrough", Qt::CaseInsensitive) == 0)
	{
		CloseInlineTag(QStringLiteral("</s>"), inStrike);
		return true;
	}
	if (name.compare("sub", Qt::CaseInsensitive) == 0)
	{
		CloseInlineTag(QStringLiteral("</sub>"), inSub, true);
		return true;
	}
	if (name.compare("sup", Qt::CaseInsensitive) == 0)
	{
		CloseInlineTag(QStringLiteral("</sup>"), inSup, true);
		return true;
	}
	if (name.compare("style", Qt::CaseInsensitive) == 0)
	{
		inStyle = false;
		ActiveBuffer().append(styleCloseTag);
		styleOpenTag.clear();
		styleCloseTag.clear();
		inlineWordChars     = 0;
		needSpaceBeforeText = false;
		afterTightInline    = true;
		return true;
	}
	return false;
}

} // namespace HomeCompa::Util
