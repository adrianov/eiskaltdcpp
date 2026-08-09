// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// QXmlStreamReader SAX facade matching FLibrary's SaxParser API for FB2 parsing.
//
#pragma once

#include <limits>
#include <memory>

#include <QString>

#include "fnd/NonCopyMovable.h"

#include "export/util.h"

class QIODevice;

namespace HomeCompa {
namespace Util
{

class XmlAttributes;

class UTIL_EXPORT SaxParser
{
    NON_COPY_MOVABLE(SaxParser)

protected:
    explicit SaxParser(QIODevice& stream, int64_t maxChunkSize = std::numeric_limits<int64_t>::max());
    virtual ~SaxParser();

public:
    void Parse();

public:
    virtual bool OnProcessingInstruction(const QString& target, const QString& data);
    virtual bool OnXMLDecl(const QString& versionStr, const QString& encodingStr, const QString& standaloneStr, const QString& actualEncodingStr);

    virtual bool OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes);
    virtual bool OnEndElement(const QString& name, const QString& path);
    virtual bool OnCharacters(const QString& path, const QString& value);

    virtual bool OnWarning(size_t line, size_t column, const QString& text);
    virtual bool OnError(size_t line, size_t column, const QString& text);
    virtual bool OnFatalError(size_t line, size_t column, const QString& text);

    bool IsLastItemProcessed() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

protected:
    bool m_processed { true };
};

} // namespace Util
} // namespace HomeCompa
