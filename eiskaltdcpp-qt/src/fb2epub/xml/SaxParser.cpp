// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// QXmlStreamReader SAX facade for FB2→EPUB parsing.
//
#include "SaxParser.h"

#include <QIODevice>
#include <QStringList>
#include <QXmlStreamReader>

#include "XmlAttributes.h"

namespace HomeCompa {
namespace Util
{

namespace
{

class XmlAttributesImpl final : public XmlAttributes
{
public:
    void setAttributes(const QXmlStreamAttributes& attributes)
    {
        m_attributes = attributes;
    }

private:
    QString GetAttribute(const QString& key) const override
    {
        for (const auto& attr : m_attributes) {
            if (attr.name().compare(key, Qt::CaseInsensitive) == 0
                    || attr.qualifiedName().compare(key, Qt::CaseInsensitive) == 0)
                return attr.value().toString();
        }
        return {};
    }

    size_t GetCount() const override
    {
        return static_cast<size_t>(m_attributes.size());
    }

    QString GetName(const size_t index) const override
    {
        return m_attributes.at(static_cast<int>(index)).qualifiedName().toString();
    }

    QString GetValue(const size_t index) const override
    {
        return m_attributes.at(static_cast<int>(index)).value().toString();
    }

private:
    QXmlStreamAttributes m_attributes;
};

class XmlStack
{
public:
    void Push(const QString& tag)
    {
        m_data.push_back(tag);
        m_keyValid = false;
    }

    void Pop(const QString& tag)
    {
        if (m_data.isEmpty() || m_data.back() != tag)
            return;
        m_data.pop_back();
        m_keyValid = false;
    }

    const QString& ToString() const
    {
        if (!m_keyValid) {
            m_key = m_data.join(QLatin1Char('/'));
            m_keyValid = true;
        }
        return m_key;
    }

private:
    mutable bool m_keyValid = false;
    mutable QString m_key;
    QStringList m_data;
};

} // namespace

class SaxParser::Impl
{
public:
    Impl(SaxParser& self, QIODevice& stream)
        : m_self(self)
        , m_reader(&stream)
    {
        m_reader.setNamespaceProcessing(true);
    }

    void Parse()
    {
        XmlStack stack;
        XmlAttributesImpl attributes;
        QString characters;
        bool stopped = false;

        auto flushCharacters = [&]() {
            if (stopped)
                return;
            if (characters.simplified().isEmpty()) {
                characters.clear();
                return;
            }
            if (!m_self.OnCharacters(stack.ToString(), characters))
                stopped = true;
            characters.clear();
        };

        while (!stopped && !m_reader.atEnd()) {
            switch (m_reader.readNext()) {
            case QXmlStreamReader::ProcessingInstruction:
                flushCharacters();
                if (!m_self.OnProcessingInstruction(m_reader.processingInstructionTarget().toString(),
                                                     m_reader.processingInstructionData().toString()))
                    stopped = true;
                break;
            case QXmlStreamReader::StartDocument:
                if (!m_self.OnXMLDecl(m_reader.documentVersion().toString(),
                                      m_reader.documentEncoding().toString(),
                                      QString(),
                                      QString()))
                    stopped = true;
                break;
            case QXmlStreamReader::StartElement: {
                flushCharacters();
                const QString name = m_reader.name().toString();
                stack.Push(name);
                attributes.setAttributes(m_reader.attributes());
                if (!m_self.OnStartElement(name, stack.ToString(), attributes))
                    stopped = true;
                break;
            }
            case QXmlStreamReader::EndElement: {
                flushCharacters();
                const QString name = m_reader.name().toString();
                if (!m_self.OnEndElement(name, stack.ToString()))
                    stopped = true;
                stack.Pop(name);
                break;
            }
            case QXmlStreamReader::Characters:
                if (!m_reader.isWhitespace())
                    characters.append(m_reader.text());
                break;
            case QXmlStreamReader::Invalid:
                if (!m_self.OnFatalError(static_cast<size_t>(m_reader.lineNumber()),
                                         static_cast<size_t>(m_reader.columnNumber()),
                                         m_reader.errorString()))
                    stopped = true;
                break;
            default:
                break;
            }
        }

        flushCharacters();
    }

private:
    SaxParser& m_self;
    QXmlStreamReader m_reader;
};

SaxParser::SaxParser(QIODevice& stream, const int64_t /*maxChunkSize*/)
    : m_impl(std::make_unique<Impl>(*this, stream))
{
}

SaxParser::~SaxParser() = default;

void SaxParser::Parse()
{
    m_impl->Parse();
}

bool SaxParser::IsLastItemProcessed() const noexcept
{
    return m_processed;
}

bool SaxParser::OnProcessingInstruction(const QString&, const QString&)
{
    return true;
}

bool SaxParser::OnXMLDecl(const QString&, const QString&, const QString&, const QString&)
{
    return true;
}

bool SaxParser::OnStartElement(const QString&, const QString&, const XmlAttributes&)
{
    return true;
}

bool SaxParser::OnEndElement(const QString&, const QString&)
{
    return true;
}

bool SaxParser::OnCharacters(const QString&, const QString&)
{
    return true;
}

bool SaxParser::OnWarning(size_t, size_t, const QString&)
{
    return true;
}

bool SaxParser::OnError(size_t, size_t, const QString&)
{
    return false;
}

bool SaxParser::OnFatalError(size_t, size_t, const QString&)
{
    return false;
}

} // namespace Util
} // namespace HomeCompa
