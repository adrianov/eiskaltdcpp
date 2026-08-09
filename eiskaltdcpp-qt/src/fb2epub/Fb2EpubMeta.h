// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#pragma once

#include <QString>
#include <QStringList>
#include <vector>

namespace HomeCompa::Util
{

struct Fb2Person
{
	QString first;
	QString middle;
	QString last;
	QString nickname;
};

struct Fb2PublishInfo
{
	QString bookName;
	QString publisher;
	QString city;
	QString year;
	QString isbn;
};

struct Fb2Sequence
{
	QString name;
	QString number;
};

struct Fb2Metadata
{
	std::vector<Fb2Person> authors;
	std::vector<Fb2Person> translators;
	std::vector<Fb2Person> sourceAuthors;
	QString                annotation;
	QString                sourceTitle;
	QStringList            keywords;
	QStringList            genres;
	QString                sourceLanguage;
	Fb2PublishInfo         publishInfo;
	Fb2Sequence            sequence;
	QString                documentId;
	QString                documentDate;
};

[[nodiscard]] QString FormatFb2PersonName(const Fb2Person& person);
[[nodiscard]] QString FormatFb2PersonFileAs(const Fb2Person& person);
[[nodiscard]] QString PrimaryAuthorName(const Fb2Metadata& metadata);
[[nodiscard]] QString BuildOpfMetadata(
	const Fb2Metadata& metadata,
	const QString&     title,
	const QString&     language,
	bool               hasCover
);

} // namespace HomeCompa::Util
