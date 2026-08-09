// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion, ported from FLibrary for eiskaltdcpp-qt.
//
#include "Fb2EpubConverter.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>

#include "Fb2EpubParser.h"
#include "Fb2EpubText.h"
#include "Fb2EpubWriter.h"

namespace HomeCompa::Util
{

namespace
{

void ApplyCoverFallback(ParsedFb2& parsed, const Fb2ToEpubOptions* options)
{
    if (!options || !parsed.coverData.isEmpty() || !options->coverOverride)
        return;
    if (options->coverOverride->data.isEmpty())
        return;

    parsed.coverData = options->coverOverride->data;
    parsed.coverMime = options->coverOverride->mime.isEmpty()
        ? CoverMimeFromData(parsed.coverData)
        : options->coverOverride->mime;
}

bool ConvertParsedToEpub(const ParsedFb2& parsed, const QString& title, const QString& epubPath)
{
    const auto epubBytes = BuildEpubBytes(parsed, title);
    return !epubBytes.isEmpty() && WriteEpubBytesToFile(epubBytes, epubPath);
}

bool ConvertFb2StreamToEpub(QIODevice& stream, const QString& fallbackTitle, const QString& epubPath, const Fb2ToEpubOptions* options)
{
    ParsedFb2 parsed;
    if (!ParseFb2(stream, parsed))
        return false;

    ApplyCoverFallback(parsed, options);

    const auto title = parsed.title.isEmpty() ? fallbackTitle : parsed.title;
    return ConvertParsedToEpub(parsed, title, epubPath);
}

} // namespace

QString EpubPathForFb2(const QString& fb2Path)
{
    return QFileInfo(fb2Path).path() + "/" + QFileInfo(fb2Path).completeBaseName() + ".epub";
}

bool ConvertFb2ToEpub(const QString& fb2Path, const QString& epubPath, const Fb2ToEpubOptions* options)
{
    if (fb2Path.isEmpty() || epubPath.isEmpty())
        return false;

    QFile fb2(fb2Path);
    if (!fb2.open(QIODevice::ReadOnly))
        return false;

    return ConvertFb2StreamToEpub(fb2, QFileInfo(fb2Path).completeBaseName(), epubPath, options);
}

bool ConvertFb2BytesToEpub(const QByteArray& fb2Bytes, const QString& fb2FileName, const QString& epubPath, const Fb2ToEpubOptions* options)
{
    if (fb2Bytes.isEmpty() || fb2FileName.isEmpty() || epubPath.isEmpty())
        return false;

    QBuffer stream;
    stream.setData(fb2Bytes);
    if (!stream.open(QIODevice::ReadOnly))
        return false;

    return ConvertFb2StreamToEpub(stream, QFileInfo(fb2FileName).completeBaseName(), epubPath, options);
}

} // namespace HomeCompa::Util
