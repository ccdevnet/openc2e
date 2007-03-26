#include "s16Image.h"
#include "endianlove.h"
#include <fstream>
#include <cstdio>

S16Sprite::~S16Sprite() {
    if (!foreign)
        free(m_pixel_data);
}


// XXX: check for running off the end of the mmap buffer
void S16Sprite::regenerate() {

#define CHECK_P(p) do { \
    void *p__ = (p); \
    if (p__ > (is->map + is->filesize - sizeof(*(p)))) \
        throw imageFormatException("buffer underrun"); \
} while(0)

    // Actually load the sprite
    if (!is_c16) {
        uint16_t *data_p;
        data_p = (uint16_t *)(is->map + *(uint32_t *)(is->map + header_offset));
        if (is_bg)
            data_p += 2; // 4 bytes offset

        // XXX: unsigned long long isn't needed on 32-bit, what's a portable
        // int of the right size?
        bool aligned = ((unsigned long long)(data_p) & 1 == 0);

        if (data_p + m_width * m_height > (uint16_t *)(is->map + is->filesize))
            throw imageFormatException("buffer underrun");
        if (IS_LITTLE_ENDIAN && aligned) {
            m_pixel_data = data_p;
            foreign = true;
        } else {
            uint16_t *copy_p = (uint16_t *)malloc(m_width * m_height * 2);
            m_pixel_data = copy_p;
            foreign = false;

            if (aligned) {
                for (unsigned int i = 0; i < m_width * m_height; i++)
                    copy_p[i] = swapEndianShort(data_p[i]);
            } else {
                uint16_t *tempbuf = (uint16_t *)malloc(m_width * 2);
                for (unsigned int y = 0; y < m_height; y++) {
                    memcpy((void *)tempbuf, (void *)(data_p + m_width * y),
                            m_width * 2);
                    for (unsigned int x = 0; x < m_width; x++)
                        copy_p[m_width * y + x] = swapEndianShort(tempbuf[x]);
                }
                free(tempbuf);
            }
        }
    } else { // C16
        uint16_t *dest_p = (uint16_t *)malloc(m_width * m_height * 2);
        m_pixel_data = (void *)dest_p;
        foreign = false;

        void *header_p = alloca(4 + 4 * m_height);
        memcpy(header_p, (void *)(is->map + header_offset), 4 + 4 * m_height);

        unsigned int y;
        for (y = 0; y < m_height; y++) {
            unsigned int x = 0;
            uint32_t *line_ref_p = (uint32_t *)header_p;
            if (y != 0)
                line_ref_p += 1 + y; // skip width/height
            uint32_t line_offset = swapEndianLong(*line_ref_p);

            uint16_t *line_p = (uint16_t *)(is->map + *line_ref_p);
            uint16_t *dest_line = dest_p + (m_width * y);

            while (x < m_width) {
                CHECK_P(line_p);
                uint16_t tag = swapEndianShort(*(line_p++));
                uint16_t run_length = tag >> 1;
                img_assert(run_length > 0 && run_length + x <= m_width);

                if (tag & 0x0001) {
                    while(run_length--) {
                        CHECK_P(line_p);
                        dest_line[x++] = swapEndianShort(*(line_p++));
                    }
                } else {
                    for (unsigned int i = 0; i < run_length; i++)
                        dest_line[x++] = 0;
                }
            }
        }
    }
}

void S16Sprite::load(mmapifstream *is, uint32_t header_offset, bool is_565, bool is_c16, bool is_bg) {
    assert(header_offset);
    this->header_offset = header_offset;
    this->is = is;
    if (is_565)
        m_format = fmt_565;
    else
        m_format = fmt_555;
    this->is_c16 = is_c16;
    this->is_bg = is_bg;

    uint32_t header_len = 8;
    if (is_c16)
        header_len = 4 + 4 * m_height;

    if (header_offset + header_len > is->filesize)
        throw imageFormatException("buffer underrun");
    
    uint16_t headerfields[2];
    uint16_t *hp = (uint16_t *)(is->map + header_offset + 4);
    memcpy(&headerfields, hp, sizeof headerfields); // handle unaligned access
    m_width = swapEndianShort(headerfields[0]);
    m_height = swapEndianShort(headerfields[1]);

}

bool S16Sprite::transparentAt(unsigned int x, unsigned int y) {
    if (x == width() || y == height())
        return true; // XXX: this shouldn't happen, but does
    assert(x < width()); assert(y < height());
    return !is_bg && ((const uint16_t *)pixel_data())[y * width() + x] == 0;
}

#define FLAG_565 1
#define FLAG_C16 2

uint32_t s16Gallery::readHeader(mmapifstream *is) {
    uint32_t flags;
    uint16_t spriteCount;

    if (is->filesize < 6)
        throw imageFormatException("buffer underrun");
    flags = swapEndianShort(*(uint32_t *)is->map);
    spriteCount = swapEndianShort(*(uint16_t *)(is->map + 4));
    
    sprites.resize(spriteCount);

    is_565 = !!(flags & FLAG_565);
    is_c16 = !!(flags & FLAG_C16);

    return 6; // offset of sprite headers
}

s16Gallery::s16Gallery(mmapifstream *is) {
    int off = readHeader(is);

    for (unsigned int i = 0; i < spriteCount(); i++) {
        sprites[i].set_parent(this);
        sprites[i].load(is, off, is_565, is_c16);
        if (is_c16)
            off+= 4 + 4 * sprites[i].height();
        else
            off+= 8;
    }
}

uint32_t blkGallery::readHeader(mmapifstream *is) {
    uint32_t flags;
    uint16_t spriteCount;

    if (is->filesize < 10)
        throw imageFormatException("buffer underrun");
    flags = swapEndianShort(*(uint32_t *)is->map);
    m_width = swapEndianShort(*(uint16_t *)(is->map + 4));
    m_height = swapEndianShort(*(uint16_t *)(is->map + 6));
    spriteCount = swapEndianShort(*(uint16_t *)(is->map + 8));
    
    if (spriteCount != m_width * m_height)
        throw imageFormatException("sprite count does not match width * height");
    
    sprites.resize(spriteCount);

    is_565 = !!(flags & FLAG_565);
    is_c16 = !!(flags & FLAG_C16);

    return 10; // offset of sprite headers
}

blkGallery::blkGallery(mmapifstream *is) {
    uint32_t offset = readHeader(is);

    for (unsigned int i = 0; i < spriteCount(); i++) {
        sprites[i].set_parent(this);
        sprites[i].load(is, offset, is_565, is_c16, true);
        if (is_c16)
            offset += 4 + 4 * sprites[i].height();
        else
            offset += 8;
        if(sprites[i].height() != 128)
            throw imageFormatException("incorrect height for BLK sprite");
        if(sprites[i].width() != 128)
            throw imageFormatException("incorrect width for BLK sprite");
    }
}

class s16Codec : public imageCodec {
    public:
        boost::shared_ptr<spriteGallery> load(mmapifstream *is) const {
            s16Gallery *g = new s16Gallery(is);
            return gallery_p(g);
        }
		void save(std::ofstream &s, gallery_p &) const {
            throw creaturesException("S16 codec does not yet support saving.");
        }
};

static const s16Codec s16_codec_impl;

class blkCodec : public imageCodec {
    public:
        gallery_p load(mmapifstream *is) const {
            blkGallery *g = new blkGallery(is);
            return gallery_p(g);
        }
		void save(std::ofstream &s, gallery_p &) const {
            throw creaturesException("S16 codec does not yet support saving.");
        }
};

static const blkCodec blk_codec_impl;

const codec_p s16_codec = &s16_codec_impl;
const codec_p c16_codec = &s16_codec_impl;
const codec_p blk_codec = &blk_codec_impl;

