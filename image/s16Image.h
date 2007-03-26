#ifndef S16IMAGE_H
#define S16IMAGE_H 1

#include "image/util.h"
#include "endianlove.h"

class S16Sprite : public cachedImage {
    protected:
        mmapifstream *is;

        uint16_t m_width, m_height;
        imageFormat m_format;
        uint32_t header_offset;
        bool is_c16, foreign, is_bg;

        void regenerate();
    public:
        ~S16Sprite();

        int width() { return m_width; }
        int height() { return m_height; }
        imageFormat format() { return m_format; }

        virtual bool transparentAt(unsigned int x, unsigned int y);

        S16Sprite()
            : is(NULL), m_width(0), m_height(0), m_format(fmt_bad),
              header_offset(0), is_c16(false), foreign(true)
            { }

        void load(mmapifstream *is, uint32_t header_offset, bool is_565, bool is_c16, bool is_bg = false);
};

class s16Gallery : public vectorGallery<S16Sprite> {
    protected:
        bool is_565, is_c16;
        uint32_t readHeader(mmapifstream *is);
    public:
        s16Gallery(mmapifstream *is);
};

class blkGallery : public vectorGallery<S16Sprite, backgroundGallery> {
    protected:
        bool is_565, is_c16;
        uint16_t m_width, m_height;
        uint32_t readHeader(mmapifstream *is);
    public:
        blkGallery(mmapifstream *is);
        uint16_t width() const { return m_width; }
        uint16_t height() const { return m_height; }
};

extern const codec_p s16_codec;
extern const codec_p c16_codec;
extern const codec_p blk_codec;

#endif

