// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubParserImpl.h"

namespace HomeCompa::Util
{

namespace
{

QString ExtraBodyClass(const QString& bodyName)
{
	QString safe = bodyName.trimmed();
	for (auto& ch : safe)
	{
		if (!ch.isLetterOrNumber() && ch != '-' && ch != '_')
			ch = '_';
	}
	return safe.isEmpty() ? QStringLiteral("extra") : safe;
}

} // namespace

bool Fb2Parser::OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes)
{
	if (name.compare("body", Qt::CaseInsensitive) == 0)
	{
		const auto bodyName = attributes.GetAttribute("name");
		if (bodyName.compare("notes", Qt::CaseInsensitive) == 0)
		{
			inNotesBody = true;
			inBody      = true;
			return true;
		}
		if (!sawMainBody)
		{
			sawMainBody = true;
			inMainBody  = true;
			inBody      = true;
			ResetBodyChar();
			return true;
		}

		inExtraBody = true;
		inMainBody  = true;
		inBody      = true;
		ResetBodyChar();
		bodyBuffer.append(QString("<section class=\"fb2-body-%1\">\n").arg(ExtraBodyClass(bodyName)));
		return true;
	}

	if (name.compare("binary", Qt::CaseInsensitive) == 0)
	{
		currentBinaryId   = attributes.GetAttribute("id").trimmed();
		currentBinaryMime = attributes.GetAttribute("content-type");
		inBinary          = true;
		binaryBuffer.clear();
		return true;
	}

	if (!inBody)
		return OnMetaStartElement(name, path, attributes);

	return OnBodyStartElement(name, attributes);
}

void Fb2Parser::CommitBodyImage()
{
	if (currentImageId.isEmpty())
		return;

	bodyImageIds.push_back(currentImageId);
	if (!currentImageAlt.trimmed().isEmpty())
		imageAlts.insert(currentImageId, currentImageAlt.trimmed());
	bodyBuffer.append(QString("{{FB2IMG:%1}}\n").arg(currentImageId));
	currentImageId.clear();
	currentImageAlt.clear();
	inBodyImage         = false;
	inImageDescription  = false;
}

} // namespace HomeCompa::Util
