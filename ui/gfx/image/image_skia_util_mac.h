// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_IMAGE_IMAGE_SKIA_UTIL_MAC_H_
#define UI_GFX_IMAGE_IMAGE_SKIA_UTIL_MAC_H_

#include <ApplicationServices/ApplicationServices.h>

#include "base/component_export.h"

using NSSize = CGSize;

#ifdef __OBJC__
@class NSImage;
#else
class NSImage;
#endif

namespace gfx {
class ImageSkia;

// Converts to ImageSkia from NSImage.
COMPONENT_EXPORT(GFX) gfx::ImageSkia ImageSkiaFromNSImage(NSImage* image);

// Resizes NSImage to |size| DIP and then converts to ImageSkia.
COMPONENT_EXPORT(GFX)
gfx::ImageSkia ImageSkiaFromResizedNSImage(NSImage* image, NSSize size);

// Converts to NSImage from ImageSkia. Uses the sRGB color space.
COMPONENT_EXPORT(GFX)
NSImage* NSImageFromImageSkia(const gfx::ImageSkia& image_skia);

// Converts to NSImage from given ImageSkia.
COMPONENT_EXPORT(GFX)
NSImage* NSImageFromImageSkia(const gfx::ImageSkia& image_skia);

}  // namespace gfx

#endif  // UI_GFX_IMAGE_IMAGE_SKIA_UTIL_MAC_H_
