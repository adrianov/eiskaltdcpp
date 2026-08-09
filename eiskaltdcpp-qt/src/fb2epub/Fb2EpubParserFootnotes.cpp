// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubParserImpl.h"

#include "Fb2EpubText.h"

#include <QRegularExpression>

namespace HomeCompa::Util
{

void Fb2Parser::RegisterFootnoteAliases(const QString& id, const QString& title)
{
	if (id.isEmpty())
		return;

	const auto add = [&](const QString& alias) {
		if (!alias.isEmpty() && alias != id)
			footnoteAliases.insert(alias, id);
	};

	add(title);
	if (id.startsWith(QLatin1String("n"), Qt::CaseInsensitive) && id.size() > 1)
		add(id.mid(1));
	if (id.startsWith(QLatin1String("note_"), Qt::CaseInsensitive))
		add(id.mid(5));
}

QString Fb2Parser::ResolveFootnoteKey(const QString& fragment) const
{
	if (footnotes.contains(fragment))
		return fragment;

	if (footnoteAliases.contains(fragment))
		return footnoteAliases.value(fragment);

	const QStringList tryKeys = {
		QStringLiteral("n%1").arg(fragment),
		QStringLiteral("note_%1").arg(fragment),
		QStringLiteral("note%1").arg(fragment),
	};
	for (const auto& key : tryKeys)
	{
		if (footnotes.contains(key))
			return key;
	}

	return fragment;
}

void Fb2Parser::FixFootnoteReferences()
{
	static const QRegularExpression link(QStringLiteral(R"re(href="#fn-([^"]+)")re"));
	QString fixed;
	fixed.reserve(bodyBuffer.size());
	int last = 0;

	for (auto it = link.globalMatch(bodyBuffer); it.hasNext();)
	{
		const auto match = it.next();
		fixed.append(bodyBuffer.mid(last, match.capturedStart(0) - last));
		const auto key = ResolveFootnoteKey(match.captured(1));
		if (footnotes.contains(key))
			fixed.append(QStringLiteral("href=\"#fn-%1\"").arg(key));
		else
			fixed.append(match.captured(0));
		last = match.capturedEnd(0);
	}
	fixed.append(bodyBuffer.mid(last));
	bodyBuffer = std::move(fixed);
}

void Fb2Parser::FixFootnoteLinkMarkers()
{
	const auto unicode = QRegularExpression::UseUnicodePropertiesOption;

	// Publisher-style pop-up notes: <sup><a epub:type="noteref">n</a></sup> (sup wraps a, never the reverse).
	static const QRegularExpression legacyBracketed(
		QStringLiteral(R"re(\[\s*<a epub:type="noteref" href="#fn-([^"]+)">(\d+)</a>\s*\])re"), unicode);
	bodyBuffer.replace(legacyBracketed, QStringLiteral(R"re(<sup><a epub:type="noteref" href="#fn-\1">\2</a></sup>)re"));

	static const QRegularExpression bracketed(
		QStringLiteral(R"re(<a epub:type="noteref" href="#fn-([^"]+)">\s*\[(\d+)\]\s*</a>)re"), unicode);
	bodyBuffer.replace(bracketed, QStringLiteral(R"re(<sup><a epub:type="noteref" href="#fn-\1">\2</a></sup>)re"));

	static const QString supMarker = QStringLiteral("\x01SUPNOTE\x01");
	bodyBuffer.replace(QStringLiteral("<sup><a epub:type=\"noteref\""), supMarker);

	static const QRegularExpression bare(
		QStringLiteral(R"re(<a epub:type="noteref" href="#fn-([^"]+)">(\d+)</a>)re"), unicode);
	bodyBuffer.replace(bare, QStringLiteral(R"re(<sup><a epub:type="noteref" href="#fn-\1">\2</a></sup>)re"));

	bodyBuffer.replace(supMarker, QStringLiteral("<sup><a epub:type=\"noteref\""));

	// Superscript markers attach to the preceding word (Pourquoi<sup>1</sup>, not Pourquoi <sup>1</sup>).
	static const QRegularExpression spaceBeforeSup(
		QStringLiteral(R"re((\p{L}|\d) (<sup><a epub:type="noteref"))re"), unicode);
	bodyBuffer.replace(spaceBeforeSup, QStringLiteral(R"re(\1\2)re"));

	static const QRegularExpression wordAfterSup(
		QStringLiteral(R"re(</sup>(\p{L}))re"), unicode);
	bodyBuffer.replace(wordAfterSup, QStringLiteral(R"re(</sup> \1)re"));

	// Note number flush to following punctuation (impressa⁵, not impressa⁵ ,). Parser may
	// insert a gap after legacy [n] markers before <em>, or before , » etc.
	bodyBuffer.replace(QStringLiteral("</sup> <em>"), QStringLiteral("</sup><em>"));
	static const QRegularExpression supSpacePunct(
		QStringLiteral(R"re(</sup>\s+([,.;:!?»«]))re"), unicode);
	bodyBuffer.replace(supSpacePunct, QStringLiteral(R"re(</sup>\1)re"));
}

void Fb2Parser::AppendFootnotesHtml()
{
	if (footnotes.isEmpty())
		return;

	for (auto it = footnotes.constBegin(); it != footnotes.constEnd(); ++it)
	{
		const auto& note = it.value();
		bodyBuffer.append(QStringLiteral("<aside epub:type=\"footnote\" id=\"fn-%1\">").arg(EscapeXmlText(it.key())));
		if (note.contains(QStringLiteral("<p>")) || note.contains(QStringLiteral("<blockquote")))
			bodyBuffer.append(note);
		else
		{
			bodyBuffer.append(QStringLiteral("<p>"));
			bodyBuffer.append(note);
			bodyBuffer.append(QStringLiteral("</p>"));
		}
		bodyBuffer.append(QStringLiteral("</aside>\n"));
	}
}

QString Fb2Parser::FootnoteLink(const XmlAttributes& attributes) const
{
	const auto type      = attributes.GetAttribute("type");
	const auto xlinkType = attributes.GetAttribute("xlink:type");
	if (type.compare("note", Qt::CaseInsensitive) != 0 && xlinkType.compare("note", Qt::CaseInsensitive) != 0)
		return {};

	const auto href = FirstAttr(attributes, { "xlink:href", "l:href", "href" });
	if (href.size() <= 1 || !href.startsWith('#'))
		return {};

	return QStringLiteral("#fn-%1").arg(href.mid(1));
}

} // namespace HomeCompa::Util
