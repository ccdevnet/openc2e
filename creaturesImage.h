/*
 *  creaturesImage.h
 *  openc2e
 *
 *  Created by Alyssa Milburn on Sat May 22 2004.
 *  Copyright (c) 2004 Alyssa Milburn. All rights reserved.
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

#ifndef _CREATURESIMAGE_H
#define _CREATURESIMAGE_H

#include <string>
#include <map>
#include <fstream>
#include "endianlove.h"
#include <cassert>
#include <iostream>
#include <cstdlib>
#include "exceptions.h"
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "mmapifstream.h"
#include <fstream>

class imageFormatException : public creaturesException {
	public:
		imageFormatException() throw()
			: creaturesException("imageFormatException")
		{}

		imageFormatException(const char *s) throw()
			: creaturesException(s)
		{}

		imageFormatException(const std::string &s) throw()
			: creaturesException(s)
		{}
};

// C++ doesn't let me switch on an enum? oO
enum imageFormat {
	fmt_8bit = 0,
	fmt_555  = 1,
	fmt_565  = 2,
	fmt_32   = 3,
	fmt_24   = 4,
	fmt_bad  = 5
};
/*
typedef unsigned int imageFormat;
#define fmt_8bit 0
#define fmt_555 1
#define fmt_565 2
#define fmt_32 3
#define fmt_24 4
#define fmt_bad 5
*/

static int format_depth(imageFormat f) {
	switch (f) {
		case fmt_8bit:
			return 8;
		case fmt_555:
		case fmt_565:
		case fmt_24:
			return 24;
		case fmt_32:
			return 32;
		default:
			std::cerr << "Invalid image format: " << (int)f << std::endl;
			assert("bad image format" && false);
			exit(1);
	}
}

class creaturesImage;
typedef boost::intrusive_ptr<creaturesImage> image_p;

class creaturesImage {
	protected:
		friend void intrusive_ptr_add_ref(creaturesImage *);
		friend void intrusive_ptr_release(creaturesImage *);

		virtual void add_ref() = 0;
		virtual void release() = 0;
	public:
		virtual int width()  = 0;
		virtual int height() = 0;

		virtual const void *pixel_data() = 0;

		virtual imageFormat format() = 0;

		virtual ~creaturesImage() {}
		virtual bool transparentAt(unsigned int x, unsigned int y) = 0;
};

inline void intrusive_ptr_add_ref(creaturesImage *ci) {
	ci->add_ref();
}

inline void intrusive_ptr_release(creaturesImage *ci) {
	ci->release();
}

class spriteGallery : public boost::enable_shared_from_this<spriteGallery> {
	public:
		virtual int spriteCount() = 0;
		int numframes() { return spriteCount(); }
		virtual image_p getSprite(unsigned int) = 0;
		
		virtual ~spriteGallery() {}

		int width(int i) { return getSprite(i)->width(); }
		int height(int i) { return getSprite(i)->height(); }

		std::string name;

		boost::shared_ptr<spriteGallery> ref() { return shared_from_this(); }
};

typedef boost::shared_ptr<spriteGallery> gallery_p;

class mutableGallery : public spriteGallery {
	public:
		virtual void truncate(unsigned int) = 0;
		virtual void append(image_p &) = 0;
};

class imageCodec {
	public:
		virtual gallery_p load(mmapifstream *is) const = 0;
		virtual void save(std::ofstream &s, gallery_p &) const = 0;
		virtual ~imageCodec() { } // to silence warning
};

class backgroundGallery : public spriteGallery {
	public:
		virtual uint16_t width() const = 0;
		virtual uint16_t height() const = 0;
		// XXX: Should these be the virtuals?
		int totalwidth() { return width() * 128; }
		int totalheight() { return height() * 128; }

};

typedef const imageCodec *codec_p;

class imageGallery {
protected:
	std::map<std::string, gallery_p> galleries;

public:
	gallery_p getGallery(std::string name);
//	void delImage(creaturesImage *i);
};

#endif
/* vim: set noet: */
