// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2 path helpers for FB2→EPUB conversion.
//
#include "Fb2Format.h"

#include <QFile>
#include <QFileInfo>

namespace HomeCompa::Util
{

bool IsFb2Suffix(const QString& suffix)
{
	return suffix.compare(QStringLiteral("fb2"), Qt::CaseInsensitive) == 0 || suffix.compare(QStringLiteral("fbd"), Qt::CaseInsensitive) == 0;
}

bool IsFb2Path(const QString& path)
{
	return IsFb2Suffix(QFileInfo(path).suffix());
}

QStringList CollectFb2Args(const QStringList& args)
{
	QStringList paths;
	for (const auto& arg : args)
	{
		if (arg.startsWith(u'-'))
			continue;

		if (!IsFb2Path(arg) || !QFile::exists(arg))
			continue;

		paths.append(QFileInfo(arg).absoluteFilePath());
	}
	return paths;
}

bool IsEpubSuffix(const QString& suffix)
{
	return suffix.compare(QStringLiteral("epub"), Qt::CaseInsensitive) == 0;
}

bool IsEpubPath(const QString& path)
{
	return IsEpubSuffix(QFileInfo(path).suffix());
}

bool IsExportableEpubSource(const QString& fileName)
{
	if (IsFb2Path(fileName) || IsEpubPath(fileName))
		return true;

	if (!QFileInfo(fileName).suffix().isEmpty())
		return false;

	return IsFb2Path(fileName + ".fb2") || IsFb2Path(fileName + ".fbd") || IsEpubPath(fileName + ".epub");
}

QString ResolveArchiveBookFile(const QStringList& archiveFiles, const QString& bookFile)
{
	if (archiveFiles.contains(bookFile))
		return bookFile;

	const auto baseName = QFileInfo(bookFile).completeBaseName();
	if (baseName.isEmpty())
		return {};

	for (const auto* ext : { ".fb2", ".fbd", ".epub" })
	{
		const auto candidate = baseName + ext;
		if (archiveFiles.contains(candidate))
			return candidate;
	}

	return {};
}

} // namespace HomeCompa::Util
