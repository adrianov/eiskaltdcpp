// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubParserImpl.h"

#include "Fb2EpubText.h"

namespace HomeCompa {
namespace Util
{

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

bool NeedsWordBoundarySpace(QChar prev, QChar next)
{
	return IsWordChar(prev) && IsWordChar(next) && ScriptsDiffer(prev, next);
}

bool ShouldInsertWordSpace(QChar prev, QChar first, bool needSpace, bool afterTight)
{
	if (!IsWordChar(prev))
		return false;
	if (first == u'[')
		return true;
	if (!IsWordChar(first))
		return false;
	return needSpace || (!afterTight && NeedsWordBoundarySpace(prev, first));
}

void AppendSpaceUnlessPresent(QString& buffer)
{
	if (!buffer.isEmpty() && !buffer.back().isSpace())
		buffer.append(u' ');
}

QChar LastWordCharIn(const QString& text)
{
	for (int i = text.size() - 1; i >= 0; --i)
	{
		if (IsWordChar(text.at(i)))
			return text.at(i);
	}
	return {};
}

} // namespace

bool Fb2Parser::InContent() const
{
	if (inNoteTitle)
		return false;
	if (inMainBody)
		return true;
	return inNotesBody && inNoteSection;
}

QString& Fb2Parser::ActiveBuffer()
{
	if (inNotesBody && inNoteSection)
		return noteBuffer;
	return bodyBuffer;
}

void Fb2Parser::MarkInlineOpenBoundary()
{
	if (IsWordChar(lastBodyChar))
		needSpaceBeforeText = true;
}

void Fb2Parser::MarkInlineCloseBoundary()
{
	// Drop cap: single-letter <em>П</em>одумать — no space. Empty or multi-char inline — space before next word.
	needSpaceBeforeText = inlineWordChars != 1;
	inlineWordChars     = 0;
}

void Fb2Parser::AppendSpaceBeforeInlineIfNeeded()
{
}

void Fb2Parser::ResetBodyChar()
{
	lastBodyChar = QChar();
	lastWordChar = QChar();
	needSpaceBeforeText = false;
}

bool Fb2Parser::InInlineMarkup() const
{
	return inEmphasis || inStrong || inCode || inStyle;
}

void Fb2Parser::AppendBodyText(const QString& value)
{
	if (!InContent() || !InTextContent() || value.isEmpty())
		return;

	auto& buffer = ActiveBuffer();
	if (value.trimmed().isEmpty())
	{
		AppendSpaceUnlessPresent(buffer);
		return;
	}

	const auto collapsed = SplitGluedWords(CollapseWhitespace(value));
	if (collapsed.isEmpty())
		return;

	if (ShouldInsertWordSpace(lastBodyChar, collapsed.front(), needSpaceBeforeText, afterTightInline))
		AppendSpaceUnlessPresent(buffer);

	needSpaceBeforeText = false;
	afterTightInline    = false;

	if (value.front().isSpace())
		AppendSpaceUnlessPresent(buffer);
	buffer.append(EscapeHtmlText(collapsed));
	if (value.back().isSpace())
		AppendSpaceUnlessPresent(buffer);

	if (InInlineMarkup())
		inlineWordChars += collapsed.size();
	lastBodyChar = collapsed.back();
	if (const auto word = LastWordCharIn(collapsed); !word.isNull())
		lastWordChar = word;

	if (inTitle || inSubtitle)
		AppendCollapsedText(headingBuffer, value);
}

} // namespace Util
} // namespace HomeCompa
