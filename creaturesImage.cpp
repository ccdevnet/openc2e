/*
 *  creaturesImage.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Sun Jun 06 2004.
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

#include "creaturesImage.h"
#include "openc2e.h"
#include "World.h"
#include "image/s16Image.h"

#ifndef _WIN32
#include "PathResolver.h"
#else
#define resolveFile exists
#endif

#include <iostream>
#include <fstream>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#ifndef _WIN32
#include <sys/types.h> // passwd*
#include <pwd.h> // getpwuid
#endif

using namespace boost::filesystem;

enum filetype { blk, s16, c16, spr };

#ifndef _WIN32
path homeDirectory() {
	path p;
	char *envhome = getenv("HOME");
	if (envhome)
		p = path(envhome, native);
	if ((!envhome) || (!is_directory(p)))
		p = path(getpwuid(getuid())->pw_dir, native);
	if (!is_directory(p)) {
		std::cerr << "Can't work out what your home directory is, giving up and using /tmp for now." << std::endl;
		p = path("/tmp", native); // sigh
	}
	return p;
}
#else
path homeDirectory() {
	// TODO: fix this!
	path p = path("./temp", native);
	if (!exists(p))
		create_directory(p);
	return p;
}
#endif

path cacheDirectory() {
	path p = path(homeDirectory().native_directory_string() + "/.openc2e", native);
	if (!exists(p))
		create_directory(p);
	else if (!is_directory(p))
		throw creaturesException("Your .openc2e is a file, not a directory. That's bad.");
	return p;
}

bool tryOpen(mmapifstream *in, gallery_p &img, std::string fname, filetype ft) {
	path cachefile, realfile;
	std::string cachename;
	if (fname.size() < 5) return false; // not enough chars for an extension and filename..
	std::string basename = fname; basename.erase(basename.end() - 4, basename.end()); 

	// work out where the real file should be
	switch (ft) {
		case blk:
			realfile = path(world.findFile(std::string("/Backgrounds/") + fname), native); break;

		case spr:
		case c16:
		case s16:
			realfile = path(world.findFile(std::string("/Images/") + fname), native); break;
	}

	// if it doesn't exist, too bad, give up.
	if (!exists(realfile)) return false;
	
	// work out where the cached file should be
	cachename = cacheDirectory().native_directory_string() + "/" + fname;
	if (ft == c16) { // TODO: we should really stop the caller from appending .s16/.c16
		cachename.erase(cachename.end() - 4, cachename.end());
		cachename.append(".s16");
	}

	cachefile = path(cachename, native);

	if (resolveFile(cachefile)) {
		// TODO: check for up-to-date-ness
		in->clear();
		in->mmapopen(cachefile.native_file_string());
		if (ft == c16) ft = s16;
		goto done;
	}
	//std::cout << "couldn't find cached version: " << cachefile.native_file_string() << std::endl;

	in->clear();
	in->mmapopen(realfile.native_file_string());
done:
	if (in->is_open()) {
		switch (ft) {
//			case blk: img = new blkImage(in); break;
//			case c16: img = new c16Image(in); break; // this should never happen, actually, once we're done
//			case s16: img = new s16Image(in); break;
//			case spr: img = new sprImage(in); break;
			case c16:
				img = c16_codec->load(in); break;
			case s16:
				img = s16_codec->load(in); break;
			case blk:
				img = blk_codec->load(in); break;
			default:
				assert(false);
		}
		img->name = basename;
	}
	return in->is_open();
}

/*
 * Retrieve an image for rendering use. To retrieve a sprite, pass the name without
 * extension. To retrieve a background, pass the full filename (ie, with .blk).
 */
gallery_p imageGallery::getGallery(std::string name) {
	if (name.empty()) return gallery_p(); // empty sprites definitely don't exist

	// step one: see if the image is already in the gallery
	std::map<std::string, gallery_p>::iterator i = galleries.find(name);
	if (i != galleries.end()) {
		return i->second;
	}

	// step two: try opening it in .c16 form first, then try .s16 form
	mmapifstream *in = new mmapifstream();
	gallery_p img;

	if (!tryOpen(in, img, name + ".s16", s16)) {
		if (!tryOpen(in, img, name + ".c16", c16)) {
			if (!tryOpen(in, img, name + ".spr", spr)) {
				bool lasttry = tryOpen(in, img, name, blk);
				if (!lasttry) {
					std::cerr << "imageGallery couldn't find the sprite '" << name << "'" << std::endl;
					return gallery_p();
				}
				galleries[name] = img;
			} else {
				galleries[name] = img;
			}
		} else {
			galleries[name] = img;
		}
	} else {
		galleries[name] = img;
	}
	
	in->close(); // doesn't close the mmap, which we still need :)

	return img;
}

#if 0
void imageGallery::delImage(creaturesImage *in) {
	in->delRef();
	if (in->refCount() == 0) {
		delete in;
		for (std::map<std::string, creaturesImage *>::iterator i = gallery.begin(); i != gallery.end(); i++) {
			if (i->second == in) { gallery.erase(i); return; }
		}
		std::cerr << "imageGallery warning: delImage got a newly unreferenced image but it isn't in the gallery\n";
	}
}
#endif

/* vim: set noet: */
