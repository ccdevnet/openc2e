/*
 *  pray.h
 *  openc2e
 *
 *  Created by Alyssa Milburn on Mon Jan 16 2006.
 *  Copyright (c) 2006 Alyssa Milburn. All rights reserved.
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

#ifndef PRAY_H
#define PRAY_H

#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

class prayBlock;

class prayFile {
protected:
	fs::path path;
	std::ifstream file;

public:
	std::vector<prayBlock *> blocks;
	
	prayFile(fs::path filepath);
	~prayFile();
	fs::path getPath() { return path; }
	std::istream &getStream() { return file; }
};

class prayBlock {
protected:
	bool loaded;
	bool tagsloaded;
	prayFile *parent;
	unsigned char *buffer;
	
	std::streampos offset;
	bool compressed;
	unsigned int size, compressedsize;
	
public:
	prayBlock(prayFile *p);
	~prayBlock();
	void load();
	void parseTags();
	
	std::string type;
	std::string name;
	std::map<std::string, std::string> stringValues;
	std::map<std::string, int> integerValues;

	bool isLoaded() { return loaded; }
	bool tagsLoaded() { assert(loaded); return tagsloaded; }
	prayFile *getParent() { return parent; }
	std::string getType() { return type; }
	std::string getName() { return name; }
	unsigned char *getBuffer() { assert(loaded); return buffer; }
	unsigned int getSize() { return size; }
};

class prayManager {
protected:
	std::vector<prayFile *> files;

public:
	std::map<std::string, prayBlock *> blocks;

	~prayManager();
	void update();
};

#endif
/* vim: set noet: */
