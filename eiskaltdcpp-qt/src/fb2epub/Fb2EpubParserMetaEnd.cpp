// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubParserImpl.h"

#include "Fb2EpubMetaParse.h"
#include "Fb2EpubText.h"

namespace HomeCompa {
namespace Util
{

bool Fb2Parser::OnMetaEndElement(const QString& name)
{
	if (name.compare("title-info", Qt::CaseInsensitive) == 0)
	{
		inTitleInfo = false;
		return true;
	}
	if (name.compare("publish-info", Qt::CaseInsensitive) == 0)
	{
		inPublishInfo = false;
		return true;
	}
	if (name.compare("document-info", Qt::CaseInsensitive) == 0)
	{
		inDocumentInfo = false;
		return true;
	}
	if (name.compare("src-title-info", Qt::CaseInsensitive) == 0)
	{
		inSrcTitleInfo = false;
		return true;
	}
	if (name.compare("book-title", Qt::CaseInsensitive) == 0)
	{
		inBookTitle    = false;
		inSrcBookTitle = false;
		return true;
	}
	if (name.compare("lang", Qt::CaseInsensitive) == 0)
	{
		inLanguage = false;
		return true;
	}
	if (name.compare("src-lang", Qt::CaseInsensitive) == 0)
	{
		inSrcLang = false;
		return true;
	}
	if (name.compare("annotation", Qt::CaseInsensitive) == 0 && inMetaAnnotation)
	{
		inMetaAnnotation = false;
		metadata.annotation = metadata.annotation.trimmed();
		return true;
	}
	if (name.compare("p", Qt::CaseInsensitive) == 0 && inMetaAnnotation)
	{
		if (!metadata.annotation.isEmpty() && !metadata.annotation.endsWith(QStringLiteral("\n\n")))
			metadata.annotation.append(QStringLiteral("\n\n"));
		return true;
	}
	if (name.compare("keywords", Qt::CaseInsensitive) == 0)
	{
		inKeywords = false;
		return true;
	}
	if (name.compare("genre", Qt::CaseInsensitive) == 0)
	{
		inGenre = false;
		return true;
	}
	if (name.compare("author", Qt::CaseInsensitive) == 0 && inSrcAuthor)
	{
		metadata.sourceAuthors.push_back(currentPerson);
		inSrcAuthor     = false;
		currentPerson   = {};
		inFirstName     = inMiddleName = inLastName = inNickname = false;
		return true;
	}
	if (name.compare("author", Qt::CaseInsensitive) == 0 && inAuthor)
	{
		if (inTitleInfo)
			metadata.authors.push_back(currentPerson);
		inAuthor     = false;
		currentPerson = {};
		inFirstName  = inMiddleName = inLastName = inNickname = false;
		return true;
	}
	if (name.compare("translator", Qt::CaseInsensitive) == 0 && inTranslator)
	{
		metadata.translators.push_back(currentPerson);
		inTranslator  = false;
		currentPerson = {};
		inFirstName   = inMiddleName = inLastName = inNickname = false;
		return true;
	}
	if (name.compare("first-name", Qt::CaseInsensitive) == 0)
	{
		inFirstName = false;
		return true;
	}
	if (name.compare("middle-name", Qt::CaseInsensitive) == 0)
	{
		inMiddleName = false;
		return true;
	}
	if (name.compare("last-name", Qt::CaseInsensitive) == 0)
	{
		inLastName = false;
		return true;
	}
	if (name.compare("nickname", Qt::CaseInsensitive) == 0)
	{
		inNickname = false;
		return true;
	}
	if (name.compare("book-name", Qt::CaseInsensitive) == 0)
	{
		inPublishBookName = false;
		return true;
	}
	if (name.compare("publisher", Qt::CaseInsensitive) == 0)
	{
		inPublishPublisher = false;
		return true;
	}
	if (name.compare("city", Qt::CaseInsensitive) == 0)
	{
		inPublishCity = false;
		return true;
	}
	if (name.compare("year", Qt::CaseInsensitive) == 0)
	{
		inPublishYear = false;
		return true;
	}
	if (name.compare("isbn", Qt::CaseInsensitive) == 0)
	{
		inPublishIsbn = false;
		return true;
	}
	if (name.compare("id", Qt::CaseInsensitive) == 0 && inDocumentInfo)
	{
		inDocumentId = false;
		return true;
	}
	if (name.compare("date", Qt::CaseInsensitive) == 0 && inDocumentInfo)
	{
		inDocumentDate = false;
		return true;
	}
	return false;
}

bool Fb2Parser::OnMetaCharacters(const QString& value)
{
	if (value.isEmpty())
		return false;

	if (inSrcBookTitle)
	{
		AppendCollapsedText(metadata.sourceTitle, value);
		return true;
	}
	if (inBookTitle)
	{
		AppendCollapsedText(titleBuffer, value);
		return true;
	}
	if (inLanguage)
	{
		AppendCollapsedText(languageBuffer, value);
		return true;
	}
	if (inSrcLang)
	{
		AppendCollapsedText(metadata.sourceLanguage, value);
		return true;
	}
	if (inMetaAnnotation)
	{
		const auto collapsed = CollapseWhitespace(value);
		if (!collapsed.isEmpty() && !metadata.annotation.isEmpty() && !metadata.annotation.at(metadata.annotation.size() - 1).isSpace())
		{
			const QChar prev = metadata.annotation.at(metadata.annotation.size() - 1);
			const QChar next = collapsed.at(0);
			if ((prev.isLetterOrNumber() || prev == QLatin1Char(',')) && (next.isLetterOrNumber() || next == QLatin1Char('(')))
				metadata.annotation.append(QLatin1Char(' '));
		}
		AppendCollapsedText(metadata.annotation, value);
		return true;
	}
	if (inKeywords)
	{
		SplitFb2Keywords(metadata, value);
		return true;
	}
	if (inGenre)
	{
		AppendFb2Genre(metadata, value);
		return true;
	}
	if (inAuthor || inTranslator || inSrcAuthor)
	{
		if (inFirstName)
			AppendCollapsedText(currentPerson.first, value);
		else if (inMiddleName)
			AppendCollapsedText(currentPerson.middle, value);
		else if (inLastName)
			AppendCollapsedText(currentPerson.last, value);
		else if (inNickname)
			AppendCollapsedText(currentPerson.nickname, value);
		return true;
	}
	if (inPublishBookName)
	{
		AppendCollapsedText(metadata.publishInfo.bookName, value);
		return true;
	}
	if (inPublishPublisher)
	{
		AppendCollapsedText(metadata.publishInfo.publisher, value);
		return true;
	}
	if (inPublishCity)
	{
		AppendCollapsedText(metadata.publishInfo.city, value);
		return true;
	}
	if (inPublishYear)
	{
		AppendCollapsedText(metadata.publishInfo.year, value);
		return true;
	}
	if (inPublishIsbn)
	{
		AppendCollapsedText(metadata.publishInfo.isbn, value);
		return true;
	}
	if (inDocumentId)
	{
		AppendCollapsedText(metadata.documentId, value);
		return true;
	}
	if (inDocumentDate)
	{
		AppendCollapsedText(metadata.documentDate, value);
		return true;
	}
	return false;
}

} // namespace Util
} // namespace HomeCompa
