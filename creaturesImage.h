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

class creaturesImage {
private:
	unsigned int refcount;

protected:
  unsigned int m_numframes;
  unsigned short *widths, *heights;
  void **buffers;
	uint32 *offsets;
	bool is_565;
	
	std::ifstream *stream;
  
public:
	creaturesImage() { refcount = 0; stream = 0; }
	virtual ~creaturesImage() { if (stream) delete stream; }
	bool is565() { return is_565; }
  unsigned int numframes() { return m_numframes; }
  virtual unsigned int width(unsigned int frame) { return widths[frame]; }
  virtual unsigned int height(unsigned int frame) { return heights[frame]; }
  virtual void *data(unsigned int frame) { return buffers[frame]; }
  void addRef() { refcount++; }
  void delRef() { refcount--; }
  unsigned int refCount() { return refcount; }
	
	friend class fileSwapper;
};

class imageGallery {
protected:
	std::map<std::string, creaturesImage *> gallery;

public:
	creaturesImage *getImage(std::string name);
	void delImage(creaturesImage *i);
};

extern imageGallery gallery;

#endif
/* vim: set noet: */
