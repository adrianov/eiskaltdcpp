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

bool NeedsWordBoundarySpace(QChar prev, QChar next, bool /*afterInline*/)
{
	if (!IsWordChar(prev) || !IsWordChar(next))
		return false;

	return ScriptsDiffer(prev, next);
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

void Fb2Parser::AppendBodyText(const QString& value)
{
	if (!InContent() || !InTextContent())
		return;

	auto& buffer = ActiveBuffer();
	if (value.isEmpty())
		return;

	if (value.trimmed().isEmpty())
	{
		if (!buffer.isEmpty() && !buffer.back().isSpace())
			buffer.append(u' ');
		return;
	}

	const auto collapsed = SplitGluedWords(CollapseWhitespace(value));
	if (collapsed.isEmpty())
		return;

	const auto appendSpaceIfNeeded = [&]() {
		if (buffer.isEmpty() || buffer.back().isSpace())
			return;
		buffer.append(' ');
	};

	const auto first = collapsed.front();
	if (IsWordChar(lastBodyChar) && first == u'[')
		appendSpaceIfNeeded();
	else if (needSpaceBeforeText && IsWordChar(lastBodyChar) && IsWordChar(first))
		appendSpaceIfNeeded();
	else if (!afterTightInline && IsWordChar(lastBodyChar) && IsWordChar(first) && NeedsWordBoundarySpace(lastBodyChar, first, false))
		appendSpaceIfNeeded();

	needSpaceBeforeText = false;
	afterTightInline    = false;

	if (!value.isEmpty() && value.front().isSpace() && !buffer.isEmpty() && !buffer.back().isSpace())
		buffer.append(u' ');

	buffer.append(EscapeHtmlText(collapsed));

	if (!value.isEmpty() && value.back().isSpace() && !buffer.isEmpty() && !buffer.back().isSpace())
		buffer.append(u' ');
	if (inEmphasis || inStrong || inCode || inStyle)
		inlineWordChars += collapsed.size();
	lastBodyChar = collapsed.back();

	for (int i = collapsed.size() - 1; i >= 0; --i)
	{
		const auto c = collapsed.at(i);
		if (IsWordChar(c))
		{
			lastWordChar = c;
			break;
		}
	}

	if (inTitle || inSubtitle)
		AppendCollapsedText(headingBuffer, value);
}

} // namespace HomeCompa::Util
