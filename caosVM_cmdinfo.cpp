/*
 *  caosVM_cmdinfo.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Wed May 26 2004.
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

#include "caosVM_cmdinfo.h"
#include <algorithm>
#include <ctype.h>
#include <iostream>
#include <assert.h>

cmdinfo *cmds, *funcs;
std::vector<cmdinfo> doublecmds, doublefuncs;

signed char varnumber;

cmdinfo *getCmdInfo(std::string cmd, bool command) {
	// std::cerr << "getCmdInfo called with '" << cmd << "' (wanting " << (command ? "command" : "function") << ")\n";
	assert(cmd.size() == 4);
	varnumber = -1;
	transform(cmd.begin(), cmd.end(), cmd.begin(), toupper);
	if (isdigit(cmd[2])) {// handle vaxx etc
		varnumber = atoi(cmd.c_str() + 2);
		cmd[2] = 'x';
		cmd[3] = 'x';
	}
	unsigned int i = (command ?
										phash_cmd(*((int *)cmd.c_str())) :
										phash_func(*((int *)cmd.c_str())) );
	cmdinfo *x = (command ? &cmds[i] : &funcs[i]);
	if ((!x->twotokens) && (!x->method)) return 0;
	if (x->name.compare(cmd)) {
#ifdef CAOSDEBUGDETAIL
		// eventually, this should be moved to CAOSDEBUG
		// but for now, the idiot parser calls getCmdInfo with command set to both true and false
		// so we need to fix that first
		std::cerr << "getCmdInfo wanted " << cmd << " but got " << x->name << " out of the hash table!\n";
#endif
		return 0;
	}
	return x;
}

cmdinfo *getSecondCmd(cmdinfo *first, std::string cmd, bool command) {
	transform(cmd.begin(), cmd.end(), cmd.begin(), toupper);
	std::string complete = first->name + " " + cmd;
	assert(cmd.size() == 4);
	assert(first->twotokens);
	if (command) {
		for (std::vector<cmdinfo>::iterator i = doublecmds.begin(); i != doublecmds.end(); i++) {
			if (!i->name.compare(complete)) return &(*i);
		}
	}
	std::cerr << "getSecondCmd failed to find '" << complete << "' :-/\n";
	return 0;
}
