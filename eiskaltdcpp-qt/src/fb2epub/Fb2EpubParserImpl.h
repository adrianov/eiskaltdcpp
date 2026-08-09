// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//

#pragma once

#include <QByteArray>
#include <QChar>
#include <QMap>
#include <QString>
#include <initializer_list>
#include <vector>

#include <QPair>

#include "Fb2EpubParser.h"
#include "Fb2EpubMeta.h"
#include "xml/SaxParser.h"
#include "xml/XmlAttributes.h"

namespace HomeCompa::Util
{

inline QString FirstAttr(const XmlAttributes& attributes, std::initializer_list<const char*> keys)
{
	for (const char* key : keys)
	{
		const auto value = attributes.GetAttribute(key);
		if (!value.isEmpty())
			return value;
	}
	return {};
}

class Fb2Parser final : public SaxParser
{
public:
	explicit Fb2Parser(QIODevice& stream);

	[[nodiscard]] ParsedFb2 TakeResult();

private:
	bool OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes) override;
	bool OnMetaStartElement(const QString& name, const QString& path, const XmlAttributes& attributes);
	bool OnBodyStartElement(const QString& name, const XmlAttributes& attributes);
	bool OnEndElement(const QString& name, const QString& path) override;
	bool OnMetaEndElement(const QString& name);
	bool OnCharacters(const QString& path, const QString& value) override;
	bool OnMetaCharacters(const QString& value);
	void AddTocItem();

	[[nodiscard]] bool     InContent() const;
	[[nodiscard]] bool     InTextContent() const;
	[[nodiscard]] QString& ActiveBuffer();
	void                   AppendBlockHtml(const QString& html);
	void                   OpenInlineTag(const QString& openTag, bool& flag, bool tight = false);
	void                   CloseInlineTag(const QString& closeTag, bool& flag, bool tight = false);
	bool                   TryBodyExtraStart(const QString& name, const XmlAttributes& attributes);
	bool                   TryBodyExtraEnd(const QString& name);
	void                   CommitBodyImage();
	void                   MarkInlineOpenBoundary();
	void                   MarkInlineCloseBoundary();
	void                   AppendSpaceBeforeInlineIfNeeded();
	void                   ResetBodyChar();
	void                   AppendBodyText(const QString& value);
	void                   AppendFootnotesHtml();
	void                   FixFootnoteReferences();
	void                   FixFootnoteLinkMarkers();
	void                   RegisterFootnoteAliases(const QString& id, const QString& title);
	[[nodiscard]] QString  ResolveFootnoteKey(const QString& fragment) const;
	[[nodiscard]] QString  FootnoteLink(const XmlAttributes& attributes) const;

	QString                 titleBuffer;
	QString                 languageBuffer;
	Fb2Metadata             metadata;
	Fb2Person               currentPerson;
	QString                 bodyBuffer;
	QString                 noteBuffer;
	QString                 noteTitleBuffer;
	QString                 currentNoteId;
	QString                 binaryBuffer;
	QString                 currentBinaryId;
	QString                 currentBinaryMime;
	QString                 headingBuffer;
	QString                 coverId;
	QByteArray              coverData;
	QString                 coverMime;
	QMap<QString, QPair<QByteArray, QString>> binaries;
	std::vector<QString>    bodyImageIds;
	QMap<QString, QString>    imageAlts;
	QMap<QString, QString>  footnotes;
	QMap<QString, QString>  footnoteAliases;
	std::vector<Fb2TocItem> tocItems;

	bool    inTitleInfo { false };
	bool    inPublishInfo { false };
	bool    inDocumentInfo { false };
	bool    inMetaAnnotation { false };
	bool    inKeywords { false };
	bool    inGenre { false };
	bool    inTranslator { false };
	bool    inMiddleName { false };
	bool    inNickname { false };
	bool    inSrcLang { false };
	bool    inPublishBookName { false };
	bool    inPublishPublisher { false };
	bool    inPublishCity { false };
	bool    inPublishYear { false };
	bool    inPublishIsbn { false };
	bool    inDocumentId { false };
	bool    inDocumentDate { false };
	QString                 currentImageId;
	QString                 currentImageAlt;
	bool    inExtraBody { false };
	bool    inBodyImage { false };
	bool    inImageDescription { false };
	bool    inSrcTitleInfo { false };
	bool    inSrcBookTitle { false };
	bool    inSrcAuthor { false };
	bool    inMainBody { false };
	bool    inNotesBody { false };
	bool    inNoteSection { false };
	bool    inNoteTitle { false };
	bool    sawMainBody { false };
	bool    inBody { false };
	bool    inBookTitle { false };
	bool    inLanguage { false };
	bool    inAuthor { false };
	bool    inFirstName { false };
	bool    inLastName { false };
	bool    inParagraph { false };
	bool    inVerse { false };
	bool    inTitle { false };
	bool    inSubtitle { false };
	bool    inEmphasis { false };
	bool    inStrong { false };
	bool    inLink { false };
	bool    inCode { false };
	bool    inStrike { false };
	bool    inSub { false };
	bool    inSup { false };
	bool    inStyle { false };
	bool    inTextAuthor { false };
	bool    inTableCell { false };
	bool    inPoem { false };
	bool    inCite { false };
	bool    inEpigraph { false };
	bool    inAnnotation { false };
	bool    inTable { false };
	bool    subtitleAsHeading { false };
	QString styleOpenTag;
	QString styleCloseTag;
	bool    afterTightInline { false };
	bool    inBinary { false };
	int     headingLevel { 0 };
	quint64 headingCount { 0 };
	QChar   lastBodyChar;
	QChar   lastWordChar;
	bool    needSpaceBeforeText { false };
	int     inlineWordChars { 0 };
};

} // namespace HomeCompa::Util
