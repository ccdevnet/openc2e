/*
 *  mmapifstream.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Sat Jul 24 2004.
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

#include "mmapifstream.h"
#include "openc2e.h"
#ifdef _WIN32
#include <windows.h>
#else // assume POSIX
#include <sys/types.h>
#include <sys/mman.h>
#endif
#include <iostream> // debug needs only

mmapifstream::mmapifstream(std::string filename) {
	live = false;
	mmapopen(filename);
}

void mmapifstream::mmapopen(std::string filename) {
	open(filename.c_str());
	if (!is_open()) return;

#ifdef _WIN32
	// todo: store the handle somewhere?
	HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, 0, NULL);
	HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	void *mapr = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
#else
	// todo: store the FILE* somewhere?
	FILE *f = fopen(filename.c_str(), "r");
	assert(f);
/*	if (!f) {
		close();
		setstate(failbit);
		perror("fopen");
		return;
	} */

	// now do the mmap (work out filesize, then mmap)
	int fno = fileno(f);
	seekg(0, std::ios::end);
	filesize = tellg();
	assert((int)filesize != -1);
	seekg(0, std::ios::beg);

	void *mapr = mmap(0, filesize, PROT_READ, MAP_PRIVATE, fno, 0);
#endif

	assert(mapr != (void *)-1);
	map = (char *)mapr;
	live = true;
}

mmapifstream::~mmapifstream() {
	if (live)
#ifdef _WIN32
		UnmapViewOfFile(map);
#else
		munmap(map, filesize);
#endif
}
/* vim: noet : */
