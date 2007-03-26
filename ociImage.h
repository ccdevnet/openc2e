/*
 *  ociImage.h
 *  openc2e
 *
 *  Created by Bryan Donlan on Sun March 25 2007
 *  Copyright (c) 2007 Bryan Donlan. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 */


#ifndef OCIIMAGE_H
#define OCIIMAGE_H 1

#include "creaturesImage.h"
#include <istream>
#include "mmapifstream.h"

#define OCI_IMAGE_MAGIC 0x0C2E196E

/* OCI Image spec (draft)
 *
 * All words are in network order
 *
 * File header:
 * uint32_t magic = 0x0C2E196E
 * uint32_t flags = 0x00000000 (reserved)
 * uint32_t sprite_count
 * uint32_t sprite_table_offset (must be aligned to a boundrary of 4)
 *
 * At sprite_table_offset, we have a table of sprite data; each element is:
 * uint8_t encoding = (see below)
 * uint8_t format   = (see below)
 * uint16_t transparent_color = color for transparency; ignored if we have an alpha channel
 * uint32_t sprite_flags = (see below)
 * uint32_t file_offset  = an offset (from start of file) where the image data is located; must be aligned to a boundrary of 4.
 * uint16_t width
 * uint16_t height
 * uint32_t data_length = total length of image data (allows re-saving of image files whose encoding is not understood)
 * 
 * format may be:
 * OCI_FORMAT_555 = 0
 * OCI_FORMAT_565 = 1
 *
 * sprite_flags may be:
 * OCI_FORMAT_LITTLE_ENDIAN = 1
 *   If this flag is set, sprite data is encoded in little endian form,
 *   rather than in network byte order
 *
 * encoding may be:
 * OCI_ENCODING_RAW (0)
 *   a raw stream of pixel data
 * OCI_ENCODING_PACKED (1)
 *   A stream of pixel data; whenever a pixel matching transparent_color
 *   is seen, the next uint16_t indicates the number of additional pixels
 *   to repeat it
 * OCI_ENCODING_ALPHA (2)
 *     Within each scanline, we write an alpha tag, then zero or more pixels
 *   according to the tag. Each tag consists of two bytes; the first indicates
 *   the number of following pixels it applies to, the next is the 8-bit
 *   alpha value.
 *
 *   The alpha tag run length must not be zero, and must not continue past one
 *   scan line. If the alpha value is 0x00 (transparent), then the pixel data
 *   is omitted.
 */

#define OCI_FORMAT_555 0
#define OCI_FORMAT_565 1
#define OCI_LITTLE_ENDIAN 2
#define OCI_ENCODING_RAW 0


class oci_data_ref {
    protected:
        void *data;
        bool foreign;
    public:
        oci_data_ref() { data = NULL; foreign = true; }
        oci_data_ref(size_t n) { data = NULL; alloc(n); }
        oci_data_ref(void *p) { data = p; foreign = true; }

        void alloc(size_t n) {
            clear();
            data = malloc(n);
            foreign = false;
        }

        void set(void *p) {
            clear();
            data = p;
            foreign = true;
        }

        void clear() {
            if (!foreign)
                data = NULL;
        }

        void data() { return data; }

        virtual ~oci_data_ref() { clear(); }
}

struct oci_sprite {
    uint8_t encoding;
    uint8_t format;
    uint16_t transparent_color;
    uint32_t flags;
    uint32_t file_offset;
    uint16_t width;
    uint16_t height;
    uint32_t data_length;
    oci_data_ref image_data, alpha_data;
}

class ociImage : public creaturesImage {
    private:
        uint32_t flags;
        std::vector<oci_sprite> sprites;

        void readHeader();
        void loadImage(oci_sprite &);
    public:
        ociImage(mmapifstream *);
        
        bool transparentAt(unsigned int f, unsigned int x, unsigned int y) {
            uint8_t *alpha = (uint16_t *)sprites[f].alpha_data.data();
            uint16_t *image = (uint16_t *)sprites[f].image_data.data();
            unsigned int offset = (y * sprites[f].width) + x;
            if (alpha) {
                return alpha[offset] == 0;
            } else {
                return image[offset] != sprites[f].transparent_color;
            }
        }
        void *data(unsigned int frame)
            { return sprites[frame].image_data.data(); }
        void *alpha(unsigned int frame)
            { return sprites[frame].alpha_data.data(); }
        int width(unsigned int frame)
            { return sprites[frame].width(); }
        int height(unsigned int frame);
            { return sprites[frame].height(); }
        
}

#endif

