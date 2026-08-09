// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#pragma once

#include <QString>

namespace HomeCompa::Util
{

QString CollapseWhitespace(const QString& text);
QString SplitGluedWords(const QString& text);
bool    NeedsSpaceAfterPunctuation(QChar wordBeforePunct, QChar punct, QChar nextWordChar);
void    AppendCollapsedText(QString& dest, const QString& text);
void    AppendSpaceIfNeeded(QString& text);
void    AppendSeparatorIfNeeded(QString& text);
QString SanitizeXmlText(const QString& text);
QString EscapeXmlText(const QString& text);
QString EscapeHtmlText(const QString& text);
QString EpubTimestampUtc();
QString EpubContentStyles();
QString PrependBookAnnotation(const QString& bodyHtml, const QString& annotation);
QString CoverMimeFromData(const QByteArray& data);

} // namespace HomeCompa::Util
