/*
 *  ociImage.cpp
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

#include "ociImage.h"
#include "arpa/inet.h" // XXX: ntohs, ntohl

ociImage::ociImage(mmapifstream *in) {
    stream = in;
    readHeader(in);

    for (unsigned int i = 0; i < m_numframes; i++) {
        loadImage(sprites[i]);
    }
}

#define READ_LONG(d) do { \
    uint32_t v = *(uint32_t *)*stream_ptr; \
    stream_ptr += 4; \
    (d) = ntohl(v); \
} while(0)

#define READ_SHORT(d) do { \
    uint16_t v = *(uint32_t *)*stream_ptr; \
    stream_ptr += 2; \
    (d) = ntohs(v); \
} while(0)

#define READ_BYTE(d) do { \
    uint8_t v = *(uint32_t *)*(stream_ptr++); \
} while(0)

void ociImage::readHeader() {
    char *stream_ptr = stream.map;
    uint32_t magic;
    uint32_t header_offset;
    bool format_set = false;
    READ_LONG(magic);

    assert(magic == OCI_IMAGE_MAGIC); // XXX exception
    READ_LONG(flags);
    READ_LONG(m_numframes);
    READ_LONG(header_offset);

    stream_ptr = stream.map + header_offset;

    for (unsigned int i = 0; i < m_numframes; i++) {
        READ_BYTE(sprites[i].encoding);
        READ_BYTE(sprites[i].format);
        READ_SHORT(sprites[i].transparent_color);
        READ_LONG(sprites[i].flags);
        READ_LONG(sprites[i].file_offset);
        READ_SHORT(sprites[i].width);
        READ_SHORT(sprites[i].height);
        READ_LONG(sprites[i].data_length);
    }

    for (unsigned int i = 0; i < m_numframes; i++) {
        loadImage(sprites[i]);
    }
}

static void decode_raw(mmapifstream *stream, oci_sprite &sprite) {
    sprite.alpha_data.clear();
    sprite.image_data.set(stream.map + sprite.file_offset);
}

#define READ_SPRITE_SHORT(d) do { \
    uint16_t temp = *(uint16_t *)*stream_ptr; \
    stream_ptr += 2; \
    if (sprite.sprite_flags & OCI_FORMAT_LITTLE_ENDIAN) { \
        temp = swapEndianShort(temp); \
    } else { \
        temp = ntohs(temp); \
    } \
} while(0)

static void decode_packed(mmapifstream *stream, oci_sprite &sprite) {
    char *stream_ptr = stream.map + sprite.file_offset;
    sprite.image_data.alloc(sprite.width * sprite.height * 2);
    sprite.alpha_data.clear();
    
    unsigned int size = sprite.width * sprite.height;
    uint16_t *data = (uint16_t *)sprite.image_data.alloc(size * 2);
    unsigned int pos = 0;

    while (pos < size) {
        READ_SPRITE_SHORT(data[pos]);
        if (data[pos++] == sprite.transparent_color) {
            uint16_t cont;
            READ_SPRITE_SHORT(cont);
            while (cont--)
                data[pos++] = sprite.transparent_color;
        }
    }
}

static void decode_alpha(mmapifstream *stream, oci_sprite &sprite) {
    char *stream_ptr = stream.map + sprite.file_offset;
    sprite.image_data.alloc(sprite.width * sprite.height * 2);
    sprite.alpha_data.clear();
    
    unsigned int size = sprite.width * sprite.height;
    uint16_t *data = (uint16_t *)sprite.image_data.alloc(size * 2);
    uint8_t *alpha = (uint8_t *)sprite.alpha_data.alloc(size);
    unsigned int pos = 0;

    while (pos < size) {
        uint8_t scope_len, alpha;
        READ_BYTE(scope_len);
        READ_BYTE(alpha);

        assert(scope_len > 0 && scope_len + pos <= size); // XXX

        memset(alpha + pos, alpha, scope_len);
        if (alpha == 0x00) {
            memset(data + pos, 0, scope_len * 2);
        } else {
            for (unsigned int i = 0; i < scope_len; i++)
                READ_STREAM_SHORT(data[pos + i]);
        }
        pos += scope_len;
    }
}
