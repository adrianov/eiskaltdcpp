// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubParserImpl.h"

#include "Fb2EpubMeta.h"

namespace HomeCompa::Util
{

bool Fb2Parser::OnEndElement(const QString& name, const QString& /*path*/)
{
	if (OnMetaEndElement(name))
		return true;
	if (name.compare("body", Qt::CaseInsensitive) == 0)
	{
		if (inNotesBody)
			inNotesBody = false;
		else if (inExtraBody)
		{
			bodyBuffer.append("</section>\n");
			inExtraBody = false;
			inMainBody  = false;
		}
		else if (inMainBody)
			inMainBody = false;
		inBody = inMainBody || inNotesBody;
		return true;
	}
	if (name.compare("binary", Qt::CaseInsensitive) == 0 && inBinary)
	{
		inBinary = false;
		const auto data = QByteArray::fromBase64(binaryBuffer.toUtf8());
		binaryBuffer.clear();
		if (!currentBinaryId.isEmpty() && !data.isEmpty())
		{
			binaries.insert(currentBinaryId, qMakePair(data, currentBinaryMime));
			if (currentBinaryId == coverId && coverData.isEmpty())
			{
				coverData = data;
				coverMime = currentBinaryMime;
			}
		}
		currentBinaryId.clear();
		currentBinaryMime.clear();
		return true;
	}

	if (!inBody)
		return true;

	if (name.compare("description", Qt::CaseInsensitive) == 0 && inImageDescription)
	{
		inImageDescription = false;
		return true;
	}
	if (name.compare("image", Qt::CaseInsensitive) == 0 && inBodyImage)
	{
		CommitBodyImage();
		return true;
	}

	if (inNotesBody)
	{
		if (name.compare("title", Qt::CaseInsensitive) == 0 && inNoteTitle)
		{
			inNoteTitle = false;
			return true;
		}
		if (name.compare("section", Qt::CaseInsensitive) == 0 && inNoteSection)
		{
			if (!noteBuffer.isEmpty())
				footnotes[currentNoteId] = noteBuffer.trimmed();
			RegisterFootnoteAliases(currentNoteId, noteTitleBuffer.trimmed());
			currentNoteId.clear();
			noteBuffer.clear();
			noteTitleBuffer.clear();
			inNoteSection = false;
			ResetBodyChar();
			return true;
		}
	}
	else if (name.compare("section", Qt::CaseInsensitive) == 0)
	{
		AppendBlockHtml("</section>\n");
		return true;
	}

	if (!inNotesBody)
	{
		if (name.compare("title", Qt::CaseInsensitive) == 0)
		{
			inTitle = false;
			bodyBuffer.append("</h1>\n");
			AddTocItem();
			return true;
		}
		if (name.compare("subtitle", Qt::CaseInsensitive) == 0)
		{
			inSubtitle = false;
			if (subtitleAsHeading)
			{
				AppendBlockHtml("</h2>\n");
				AddTocItem();
			}
			else
				AppendBlockHtml("</p>\n");
			subtitleAsHeading = false;
			return true;
		}
	}

	if (name.compare("p", Qt::CaseInsensitive) == 0 && !(inTitle || inSubtitle))
	{
		inParagraph = false;
		AppendBlockHtml("</p>\n");
		return true;
	}
	if (name.compare("v", Qt::CaseInsensitive) == 0)
	{
		inVerse = false;
		AppendBlockHtml("<br />\n");
		return true;
	}
	if (name.compare("stanza", Qt::CaseInsensitive) == 0)
	{
		AppendBlockHtml("</p>\n");
		return true;
	}
	if (name.compare("poem", Qt::CaseInsensitive) == 0)
	{
		inPoem = false;
		AppendBlockHtml("</section>\n");
		return true;
	}
	if (name.compare("emphasis", Qt::CaseInsensitive) == 0)
	{
		inEmphasis = false;
		ActiveBuffer().append("</em>");
		MarkInlineCloseBoundary();
		return true;
	}
	if (name.compare("strong", Qt::CaseInsensitive) == 0)
	{
		inStrong = false;
		ActiveBuffer().append("</strong>");
		MarkInlineCloseBoundary();
		return true;
	}
	if (name.compare("a", Qt::CaseInsensitive) == 0 && inLink)
	{
		inLink = false;
		ActiveBuffer().append("</a>");
		MarkInlineCloseBoundary();
		return true;
	}
	TryBodyExtraEnd(name);
	return true;
}

bool ParseFb2(QIODevice& stream, ParsedFb2& result)
{
	Fb2Parser parser(stream);
	parser.Parse();
	result = parser.TakeResult();
	return !result.bodyHtml.isEmpty();
}

} // namespace HomeCompa::Util
