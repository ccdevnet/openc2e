/*
 *  imageManager.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Sun Jun 06 2004.
 *  Copyright (c) 2004-2008 Alyssa Milburn. All rights reserved.
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

#include "imageManager.h"
#include "images/c16Image.h"
#include "images/sprImage.h"
#include "images/blkImage.h"
#include "images/bmpImage.h"
#include "openc2e.h"
#include "World.h"
#include "Engine.h"
#include "fileSwapper.h"

#include "PathResolver.h"

#include <iostream>
#include <fstream>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

using namespace boost::filesystem;

enum filetype { blk, s16, c16, spr, bmp };

bool tryOpen(mmapifstream *in, shared_ptr<creaturesImage> &img, std::string fname, filetype ft) {
	path cachefile, realfile;
	std::string cachename;
	if (fname.size() < 5) return false; // not enough chars for an extension and filename..

	realfile = path(world.findFile(fname));

	// if it doesn't exist, too bad, give up.
	if (!exists(realfile)) return false;

	std::string basename = realfile.filename().string(); basename.erase(basename.end() - 4, basename.end());

	// work out where the cached file should be
	cachename = engine.storageDirectory().string() + "/" + fname;
	if (ft == c16) { // TODO: we should really stop the caller from appending .s16/.c16
		cachename.erase(cachename.end() - 4, cachename.end());
		cachename.append(".s16");
	}

#if OC2E_BIG_ENDIAN
	if (ft != spr)
		cachename = cachename + ".big";
#endif
	cachefile = path(cachename);

	if (resolveFile(cachefile)) {
		// TODO: check for up-to-date-ness
		in->clear();
		in->mmapopen(cachefile.string());
		if (ft == c16) ft = s16;
		goto done;
	}
	//std::cout << "couldn't find cached version: " << cachefile.string() << std::endl;

	in->clear();
	in->mmapopen(realfile.string());
#if OC2E_BIG_ENDIAN
	if (in->is_open() && (ft != spr)) {
		path p = cachefile.parent_path();
		if (!exists(p))
			create_directory(p);
		if (!is_directory(p))
			throw creaturesException("imageManager couldn't create cache directory '" + p.string() + "'");

		fileSwapper f;
		switch (ft) {
			case blk:
				f.convertblk(realfile.string(), cachefile.string());
				break;
			case s16:
				f.converts16(realfile.string(), cachefile.string());
				break;
			case c16:
				//cachefile = change_extension(cachefile, "");
				//cachefile = change_extension(cachefile, ".s16.big");
				f.convertc16(realfile.string(), cachefile.string());
				ft = s16;
				break;
			default:
				return true; // TODO: exception?
		}
		in->close(); // TODO: close the mmap too! how?
		if (!exists(cachefile)) return false; // TODO: exception?
		in->mmapopen(cachefile.string());
	}
#endif
done:
	if (in->is_open()) {
		switch (ft) {
			case blk: img = shared_ptr<creaturesImage>(new blkImage(in, basename)); break;
			case c16: img = shared_ptr<creaturesImage>(new c16Image(in, basename)); break; // this should never happen, actually, once we're done
			case s16: img = shared_ptr<creaturesImage>(new s16Image(in, basename)); break;
			case spr: img = shared_ptr<creaturesImage>(new sprImage(in, basename)); break;
			case bmp: img = shared_ptr<creaturesImage>(new bmpImage(in, basename)); break; // TODO: don't commit this ;p
		}
	}
	return in->is_open();
}

/*
 * Retrieve an image for rendering use. To retrieve a sprite, pass the name without
 * extension. To retrieve a background, pass the full filename (ie, with .blk).
 */
shared_ptr<creaturesImage> imageManager::getImage(std::string name, bool is_background) {
	if (name.empty()) return shared_ptr<creaturesImage>(); // empty sprites definitely don't exist

	// step one: see if the image is already in the gallery
	std::map<std::string, boost::weak_ptr<creaturesImage> >::iterator i = images.find(name);
	if (i != images.end() && i->second.lock()) {
		if (!is_background) return i->second.lock(); // TODO: handle backgrounds
	}

	// step two: try opening it in .c16 form first, then try .s16 form
	mmapifstream *in = new mmapifstream();
	shared_ptr<creaturesImage> img;

	std::string fname;
	if (is_background) {
		fname = std::string("/Backgrounds/") + name;
	} else {
		fname = std::string("/Images/") + name;
	}

	// TODO: try/catch to free the mmapifstream
	bool successful = true;
	if (engine.bmprenderer) {
		successful = tryOpen(in, img, fname + ".bmp", bmp);
	} else {
		if (is_background) {
			successful = tryOpen(in, img, fname + ".blk", blk);
		} else {
			if (!tryOpen(in, img, fname + ".s16", s16)) {
				if (!tryOpen(in, img, fname + ".c16", c16)) {
					if (!tryOpen(in, img, fname + ".spr", spr)) {
						successful = false;
					}
				}
			}
		}
	}

	if (successful) {
		if (!is_background) // TODO: handle backgrounds
			images[name] = img;
	} else {
		std::cerr << "imageGallery couldn't find the sprite '" << name << "'" << std::endl;
		delete in;
		return shared_ptr<creaturesImage>();
	}

	in->close(); // doesn't close the mmap, which we still need :)

	return img;
}

/* vim: set noet: */
