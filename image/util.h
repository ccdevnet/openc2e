#ifndef UTIL_H
#define UTIL_H 1

/*
 *  image/util.h
 *  openc2e
 *
 *  Created by Bryan Donlan on Sun Mar 25 2007.
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

#include <string>
#include <map>
#include <fstream>
#include "endianlove.h"
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <boost/smart_ptr.hpp>
#include <vector>

#include "creaturesImage.h"

class memoryImage : public creaturesImage {
	protected:
		int m_width, m_height;
		void *m_pixel_data;
		imageFormat m_format;

		uint32_t m_trans_color;

		unsigned int refcount;

		void add_ref() {
			refcount++;
		}

		void release() {
			if (!--refcount)
				delete this;
		}
	public:
		/* Note: These constructors take ownership of the pixel_data
		 * buffer, which _must_ be allocated with malloc()
		 */
		memoryImage(int w, int h, void *pixel_data, imageFormat format, uint32_t trans = 0x0000)
			: m_width(w), m_height(h),
			  m_pixel_data(pixel_data),
		      m_format(format),
			  m_trans_color(trans),
			  refcount(0)
		{}

		~memoryImage() {
			free(m_pixel_data);
		}

		int width() { return m_width; }
		int height() { return m_height; }
		const void *pixel_data() { return m_pixel_data; }
		
		virtual bool transparentAt(unsigned int x, unsigned int y) {
			int offset = y * m_width + x;
			switch (m_format) {
				case fmt_8bit: assert("fmt_8bit trans unimplemented!" && false);
				case fmt_555:
				case fmt_565:
					return ((uint16_t *)pixel_data())[offset] == m_trans_color;
				case fmt_24:
					{
						unsigned char *p = (unsigned char *)pixel_data();
						p += offset * 3;
						unsigned char r, g, b, c;
						r = p[0];
						g = p[1];
						b = p[2];
						c = (r << 16) | (g << 8) || b;
						return c == m_trans_color;
					}
				case fmt_32:
					{
						int byte_width = format_depth(m_format);
						unsigned char *p = (unsigned char *)pixel_data();
						p += offset * byte_width;
						return p[byte_width - 1] == 0x00; // alpha = 0
					}
				default:
					std::cerr << "Invalid image format: " << (int)m_format << std::endl;
					assert("bad image format" && false);
					exit(1);
			}
		}
};

class cachedImage : public creaturesImage {
	protected:
		virtual void regenerate() = 0;
        void *m_pixel_data;

        cachedImage() : m_pixel_data(NULL) {}
	public:
		virtual const void *pixel_data() {
			if (m_pixel_data)
				return m_pixel_data;
			regenerate();
			assert(m_pixel_data);
			return m_pixel_data;
		}
};

template<class T, class G = spriteGallery>
class vectorGallery : public G {
    protected:

        class sprite : public T {
            private:
                unsigned int refcount;

                vectorGallery<T, G> *parent;
                gallery_p parent_ref;
            protected:
                void add_ref() {
                    assert(parent);

                    refcount++;
                    assert(refcount);
                    if (!parent_ref.get())
                        parent_ref = parent->shared_from_this();
                }

                void release() {
                    assert(refcount);
                    assert(parent);
                    refcount--;
                    if (!refcount)
                        parent_ref.reset();
                }

            public:
                void set_parent(vectorGallery<T, G> *parent) {
                    assert(!this->parent);
                    this->parent = parent;
                }

                sprite() : parent(NULL), refcount(0) { }
        };

        std::vector<sprite> sprites;

    public:
        int spriteCount() {
            return sprites.size();
        }

        image_p getSprite(unsigned int idx) {
            assert(idx < spriteCount());
            return image_p(&sprites[idx]);
        }
};

#define img_assert(e) do { \
    if (!(e)) \
        throw imageFormatException("bad image format: " #e); \
} while(0)

#endif

