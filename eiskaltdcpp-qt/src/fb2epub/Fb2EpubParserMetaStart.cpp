// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubParserImpl.h"

#include "Fb2EpubImages.h"
#include "Fb2EpubMetaParse.h"

namespace HomeCompa::Util
{

namespace
{

constexpr auto COVERPAGE_IMAGE = "FictionBook/description/title-info/coverpage/image";

} // namespace

bool Fb2Parser::OnMetaStartElement(const QString& name, const QString& path, const XmlAttributes& attributes)
{
	if (name.compare("title-info", Qt::CaseInsensitive) == 0)
	{
		inTitleInfo = true;
		return true;
	}
	if (name.compare("src-title-info", Qt::CaseInsensitive) == 0)
	{
		inSrcTitleInfo = true;
		return true;
	}
	if (name.compare("publish-info", Qt::CaseInsensitive) == 0)
	{
		inPublishInfo = true;
		return true;
	}
	if (name.compare("document-info", Qt::CaseInsensitive) == 0)
	{
		inDocumentInfo = true;
		return true;
	}
	if (!inTitleInfo && !inPublishInfo && !inDocumentInfo && !inSrcTitleInfo)
		return true;

	if (inSrcTitleInfo)
	{
		if (name.compare("book-title", Qt::CaseInsensitive) == 0)
		{
			inSrcBookTitle = true;
			return true;
		}
		if (name.compare("author", Qt::CaseInsensitive) == 0)
		{
			inSrcAuthor   = true;
			currentPerson = {};
			return true;
		}
	}

	if (inTitleInfo)
	{
		if (coverId.isEmpty() && (path == COVERPAGE_IMAGE || name.compare("image", Qt::CaseInsensitive) == 0))
		{
			if (const auto id = RefIdFromAttr(attributes); !id.isEmpty())
				coverId = id;
			return true;
		}
		if (name.compare("genre", Qt::CaseInsensitive) == 0)
		{
			AppendFb2Genre(metadata, attributes.GetAttribute("value"));
			inGenre = true;
			return true;
		}
		if (name.compare("book-title", Qt::CaseInsensitive) == 0)
		{
			inBookTitle = true;
			return true;
		}
		if (name.compare("annotation", Qt::CaseInsensitive) == 0)
		{
			inMetaAnnotation = true;
			return true;
		}
		if (name.compare("keywords", Qt::CaseInsensitive) == 0)
		{
			inKeywords = true;
			return true;
		}
		if (name.compare("sequence", Qt::CaseInsensitive) == 0)
		{
			metadata.sequence.name   = attributes.GetAttribute("name").trimmed();
			metadata.sequence.number = attributes.GetAttribute("number").trimmed();
			return true;
		}
		if (name.compare("lang", Qt::CaseInsensitive) == 0)
		{
			inLanguage = true;
			return true;
		}
		if (name.compare("src-lang", Qt::CaseInsensitive) == 0)
		{
			inSrcLang = true;
			return true;
		}
		if (name.compare("author", Qt::CaseInsensitive) == 0)
		{
			inAuthor      = true;
			currentPerson = {};
			return true;
		}
		if (name.compare("translator", Qt::CaseInsensitive) == 0)
		{
			inTranslator  = true;
			currentPerson = {};
			return true;
		}
	}

	if (inPublishInfo)
	{
		if (name.compare("book-name", Qt::CaseInsensitive) == 0)
			inPublishBookName = true;
		else if (name.compare("publisher", Qt::CaseInsensitive) == 0)
			inPublishPublisher = true;
		else if (name.compare("city", Qt::CaseInsensitive) == 0)
			inPublishCity = true;
		else if (name.compare("year", Qt::CaseInsensitive) == 0)
			inPublishYear = true;
		else if (name.compare("isbn", Qt::CaseInsensitive) == 0)
			inPublishIsbn = true;
		return true;
	}

	if (inDocumentInfo)
	{
		if (name.compare("id", Qt::CaseInsensitive) == 0)
			inDocumentId = true;
		else if (name.compare("date", Qt::CaseInsensitive) == 0)
			inDocumentDate = true;
		return true;
	}

	if (inAuthor || inTranslator || inSrcAuthor)
	{
		if (name.compare("first-name", Qt::CaseInsensitive) == 0)
			inFirstName = true;
		else if (name.compare("middle-name", Qt::CaseInsensitive) == 0)
			inMiddleName = true;
		else if (name.compare("last-name", Qt::CaseInsensitive) == 0)
			inLastName = true;
		else if (name.compare("nickname", Qt::CaseInsensitive) == 0)
			inNickname = true;
	}
	return true;
}

} // namespace HomeCompa::Util
