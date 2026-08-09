// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubMeta.h"

#include <QUuid>

#include "Fb2EpubText.h"

namespace HomeCompa::Util
{

namespace
{

QString IsbnIdentifier(const QString& isbn)
{
	auto normalized = QString {};
	for (const auto ch : isbn)
	{
		if (ch.isDigit())
			normalized.append(ch);
	}
	if (normalized.size() == 10 || normalized.size() == 13)
		return QStringLiteral("urn:isbn:") + normalized;
	return EscapeXmlText(isbn.trimmed());
}

void AppendCreator(QString& opf, const Fb2Person& person, const QString& id, const char* role)
{
	const auto name = FormatFb2PersonName(person);
	if (name.isEmpty())
		return;

	opf += QString("    <dc:creator id=\"%1\">%2</dc:creator>\n").arg(id, EscapeXmlText(name));
	opf += QString("    <meta refines=\"#%1\" property=\"role\" scheme=\"marc:relators\">%2</meta>\n").arg(id, role);
	const auto fileAs = FormatFb2PersonFileAs(person);
	if (!fileAs.isEmpty())
		opf += QString("    <meta refines=\"#%1\" property=\"file-as\">%2</meta>\n").arg(id, EscapeXmlText(fileAs));
}

void AppendSubjects(QString& opf, const QStringList& values)
{
	for (const auto& value : values)
	{
		const auto subject = value.trimmed();
		if (!subject.isEmpty())
			opf += QString("    <dc:subject>%1</dc:subject>\n").arg(EscapeXmlText(subject));
	}
}

} // namespace

QString FormatFb2PersonName(const Fb2Person& person)
{
	if (!person.nickname.trimmed().isEmpty() && person.first.trimmed().isEmpty() && person.last.trimmed().isEmpty())
		return person.nickname.trimmed();

	QString name = person.first.trimmed();
	if (!person.middle.trimmed().isEmpty())
		name = name.isEmpty() ? person.middle.trimmed() : QString("%1 %2").arg(name, person.middle.trimmed());
	if (!person.last.trimmed().isEmpty())
		name = name.isEmpty() ? person.last.trimmed() : QString("%1 %2").arg(name, person.last.trimmed());
	return name;
}

QString FormatFb2PersonFileAs(const Fb2Person& person)
{
	const auto last = person.last.trimmed();
	if (last.isEmpty())
		return FormatFb2PersonName(person);

	QString given = person.first.trimmed();
	if (!person.middle.trimmed().isEmpty())
		given = given.isEmpty() ? person.middle.trimmed() : QString("%1 %2").arg(given, person.middle.trimmed());
	return given.isEmpty() ? last : QString("%1, %2").arg(last, given);
}

QString PrimaryAuthorName(const Fb2Metadata& metadata)
{
	return metadata.authors.empty() ? QString {} : FormatFb2PersonName(metadata.authors.front());
}

QString BuildOpfMetadata(const Fb2Metadata& metadata, const QString& title, const QString& language, bool hasCover)
{
	const auto safeTitle = EscapeXmlText(title);
	const auto safeLang  = EscapeXmlText(language.isEmpty() ? QStringLiteral("en") : language);
	const auto identifier  = QString("urn:uuid:%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

	QString opf;
	opf += "  <metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n";
	opf += QString("    <dc:identifier id=\"book-id\">%1</dc:identifier>\n").arg(identifier);

	if (!metadata.publishInfo.isbn.trimmed().isEmpty())
		opf += QString("    <dc:identifier>%1</dc:identifier>\n").arg(IsbnIdentifier(metadata.publishInfo.isbn));
	else if (!metadata.documentId.trimmed().isEmpty())
		opf += QString("    <dc:identifier>%1</dc:identifier>\n").arg(EscapeXmlText(metadata.documentId.trimmed()));

	opf += QString("    <dc:title>%1</dc:title>\n").arg(safeTitle);
	opf += QString("    <dc:language>%1</dc:language>\n").arg(safeLang);
	if (!metadata.sourceLanguage.trimmed().isEmpty() && metadata.sourceLanguage.trimmed().compare(language, Qt::CaseInsensitive) != 0)
		opf += QString("    <dc:language>%1</dc:language>\n").arg(EscapeXmlText(metadata.sourceLanguage.trimmed()));

	int creatorIndex = 0;
	for (const auto& author : metadata.authors)
		AppendCreator(opf, author, QString("author-%1").arg(++creatorIndex), "aut");
	for (const auto& translator : metadata.translators)
		AppendCreator(opf, translator, QString("translator-%1").arg(++creatorIndex), "trl");

	if (!metadata.annotation.trimmed().isEmpty())
	{
		QString description;
		for (const auto& paragraph : metadata.annotation.split(QStringLiteral("\n\n"), Qt::SkipEmptyParts))
		{
			const auto trimmed = paragraph.trimmed();
			if (trimmed.isEmpty())
				continue;
			if (!description.isEmpty())
				description.append(u' ');
			description.append(trimmed);
		}
		opf += QString("    <dc:description>%1</dc:description>\n").arg(EscapeXmlText(description));
	}

	AppendSubjects(opf, metadata.keywords);
	AppendSubjects(opf, metadata.genres);

	if (!metadata.publishInfo.publisher.trimmed().isEmpty())
		opf += QString("    <dc:publisher>%1</dc:publisher>\n").arg(EscapeXmlText(metadata.publishInfo.publisher.trimmed()));

	const auto date = !metadata.publishInfo.year.trimmed().isEmpty() ? metadata.publishInfo.year.trimmed()
	                  : !metadata.documentDate.trimmed().isEmpty()   ? metadata.documentDate.trimmed()
	                                                                 : QString {};
	if (!date.isEmpty())
		opf += QString("    <dc:date>%1</dc:date>\n").arg(EscapeXmlText(date));

	if (!metadata.sequence.name.trimmed().isEmpty())
	{
		opf += QString("    <meta property=\"belongs-to-collection\" id=\"series-1\">%1</meta>\n")
		           .arg(EscapeXmlText(metadata.sequence.name.trimmed()));
		opf += "    <meta refines=\"#series-1\" property=\"collection-type\">series</meta>\n";
		if (!metadata.sequence.number.trimmed().isEmpty())
			opf += QString("    <meta refines=\"#series-1\" property=\"group-position\">%1</meta>\n")
			           .arg(EscapeXmlText(metadata.sequence.number.trimmed()));
	}

	if (!metadata.sourceTitle.trimmed().isEmpty())
		opf += QString("    <meta property=\"dcterms:alternative\">%1</meta>\n").arg(EscapeXmlText(metadata.sourceTitle.trimmed()));
	if (!metadata.sourceAuthors.empty())
	{
		QStringList names;
		for (const auto& author : metadata.sourceAuthors)
		{
			const auto name = FormatFb2PersonName(author);
			if (!name.isEmpty())
				names.append(name);
		}
		if (!names.isEmpty())
			opf += QString("    <meta name=\"original-author\" content=\"%1\"/>\n").arg(EscapeXmlText(names.join(QStringLiteral("; "))));
	}

	opf += QString("    <meta property=\"dcterms:modified\">%1</meta>\n").arg(EpubTimestampUtc());
	if (hasCover)
		opf += "    <meta name=\"cover\" content=\"cover-image\"/>\n";
	opf += "  </metadata>\n";
	return opf;
}

} // namespace HomeCompa::Util
