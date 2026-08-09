/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

/** NSWorkspace file-type icons — QFileIconProvider needs a real path on macOS. */

#include "FileTypeIconMac.h"

#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <QImage>

namespace {

QPixmap nsImageToPixmap(NSImage *image, int pixelSide)
{
    if (!image || pixelSide <= 0)
        return QPixmap();

    // Prefer a large representation so type badges (e.g. "MP4") survive downscale.
    [image setSize:NSMakeSize(pixelSide, pixelSide)];

    QImage qimg(pixelSide, pixelSide, QImage::Format_ARGB32_Premultiplied);
    qimg.fill(0);

    CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CGContextRef ctx = CGBitmapContextCreate(
            qimg.bits(), pixelSide, pixelSide, 8, qimg.bytesPerLine(), cs,
            kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
    CGColorSpaceRelease(cs);
    if (!ctx)
        return QPixmap();

    NSGraphicsContext *gc = [NSGraphicsContext graphicsContextWithCGContext:ctx flipped:NO];
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:gc];
    [image drawInRect:NSMakeRect(0, 0, pixelSide, pixelSide)
             fromRect:NSZeroRect
            operation:NSCompositingOperationSourceOver
             fraction:1.0
       respectFlipped:YES
                hints:@{ NSImageHintInterpolation: @(NSImageInterpolationHigh) }];
    [NSGraphicsContext restoreGraphicsState];
    CGContextRelease(ctx);

    return QPixmap::fromImage(qimg);
}

NSImage *iconForExtension(const QString &ext)
{
    if (ext.isEmpty())
        return nil;
    NSString *nsExt = [NSString stringWithUTF8String:ext.toUtf8().constData()];
    if (!nsExt.length)
        return nil;

    if (@available(macOS 11.0, *)) {
        UTType *type = [UTType typeWithFilenameExtension:nsExt];
        if (type)
            return [[NSWorkspace sharedWorkspace] iconForContentType:type];
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    return [[NSWorkspace sharedWorkspace] iconForFileType:nsExt];
#pragma clang diagnostic pop
}

} // namespace

QPixmap macFileTypePixmap(const QString &ext, int pixelSide)
{
    @autoreleasepool {
        return nsImageToPixmap(iconForExtension(ext.toLower()), pixelSide);
    }
}

QPixmap macFolderPixmap(int pixelSide)
{
    @autoreleasepool {
        NSImage *image = nil;
        if (@available(macOS 11.0, *))
            image = [[NSWorkspace sharedWorkspace] iconForContentType:UTTypeFolder];
        else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            image = [[NSWorkspace sharedWorkspace]
                    iconForFileType:NSFileTypeForHFSTypeCode(kGenericFolderIcon)];
#pragma clang diagnostic pop
        }
        return nsImageToPixmap(image, pixelSide);
    }
}
