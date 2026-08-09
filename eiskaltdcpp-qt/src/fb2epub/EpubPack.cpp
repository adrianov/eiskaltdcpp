// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// In-memory EPUB ZIP packing (store + deflate), used for FB2→EPUB.
//
#include "zip.h"

#include <QByteArray>
#include <QtEndian>
#include <zlib.h>

namespace HomeCompa
{

namespace
{

quint32 crc32Of(const QByteArray& data)
{
    return static_cast<quint32>(crc32(0L, reinterpret_cast<const Bytef*>(data.constData()),
                                      static_cast<uInt>(data.size())));
}

QByteArray deflateRaw(const QByteArray& data)
{
    if (data.isEmpty())
        return {};

    uLongf bound = compressBound(static_cast<uLong>(data.size()));
    QByteArray out;
    out.resize(static_cast<int>(bound));

    z_stream strm {};
    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return {};

    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.constData()));
    strm.avail_in = static_cast<uInt>(data.size());
    strm.next_out = reinterpret_cast<Bytef*>(out.data());
    strm.avail_out = static_cast<uInt>(out.size());

    const int rc = deflate(&strm, Z_FINISH);
    const uLong produced = strm.total_out;
    deflateEnd(&strm);

    if (rc != Z_STREAM_END)
        return {};

    out.resize(static_cast<int>(produced));
    return out;
}

void appendU16(QByteArray& out, quint16 v)
{
    char buf[2];
    qToLittleEndian(v, buf);
    out.append(buf, 2);
}

void appendU32(QByteArray& out, quint32 v)
{
    char buf[4];
    qToLittleEndian(v, buf);
    out.append(buf, 4);
}

struct EntryMeta {
    QByteArray nameUtf8;
    QByteArray payload;
    quint16 method = 0;
    quint32 crc = 0;
    quint32 compSize = 0;
    quint32 uncompSize = 0;
    quint32 localOffset = 0;
};

} // namespace

QByteArray PackEpubMembers(const std::vector<std::pair<QString, QByteArray>>& members)
{
    if (members.empty() || members.front().first != QStringLiteral("mimetype"))
        return {};

    std::vector<EntryMeta> entries;
    entries.reserve(members.size());

    for (size_t i = 0; i < members.size(); ++i) {
        EntryMeta e;
        e.nameUtf8 = members[i].first.toUtf8();
        e.uncompSize = static_cast<quint32>(members[i].second.size());
        e.crc = crc32Of(members[i].second);

        if (i == 0) {
            e.method = 0;
            e.payload = members[i].second;
        } else {
            const QByteArray deflated = deflateRaw(members[i].second);
            if (!deflated.isEmpty() && deflated.size() < members[i].second.size()) {
                e.method = 8;
                e.payload = deflated;
            } else {
                e.method = 0;
                e.payload = members[i].second;
            }
        }

        e.compSize = static_cast<quint32>(e.payload.size());
        entries.push_back(std::move(e));
    }

    QByteArray out;
    for (auto& e : entries) {
        e.localOffset = static_cast<quint32>(out.size());
        appendU32(out, 0x04034b50u);
        appendU16(out, 20);
        appendU16(out, 0);
        appendU16(out, e.method);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU32(out, e.crc);
        appendU32(out, e.compSize);
        appendU32(out, e.uncompSize);
        appendU16(out, static_cast<quint16>(e.nameUtf8.size()));
        appendU16(out, 0);
        out.append(e.nameUtf8);
        out.append(e.payload);
    }

    const quint32 centralOffset = static_cast<quint32>(out.size());
    for (const auto& e : entries) {
        appendU32(out, 0x02014b50u);
        appendU16(out, 20);
        appendU16(out, 20);
        appendU16(out, 0);
        appendU16(out, e.method);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU32(out, e.crc);
        appendU32(out, e.compSize);
        appendU32(out, e.uncompSize);
        appendU16(out, static_cast<quint16>(e.nameUtf8.size()));
        appendU16(out, 0);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU32(out, 0);
        appendU32(out, e.localOffset);
        out.append(e.nameUtf8);
    }

    const quint32 centralSize = static_cast<quint32>(out.size()) - centralOffset;
    appendU32(out, 0x06054b50u);
    appendU16(out, 0);
    appendU16(out, 0);
    appendU16(out, static_cast<quint16>(entries.size()));
    appendU16(out, static_cast<quint16>(entries.size()));
    appendU32(out, centralSize);
    appendU32(out, centralOffset);
    appendU16(out, 0);

    return out;
}

} // namespace HomeCompa
