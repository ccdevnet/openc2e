/*
 *  caosVM_flow.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Sun May 30 2004.
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

#include "caosVM.h"
#include <iostream>
#include "openc2e.h"
#include "World.h" // enum
#include <cmath>   // sqrt

void caosVM::jumpToEquivalentNext() {
	// todo: add non-ENUM things
	cmdinfo *next = getCmdInfo("NEXT", true); assert(next != 0);
	cmdinfo *enu = getCmdInfo("ENUM", true); assert(enu != 0);
	cmdinfo *esee = getCmdInfo("ESEE", true); assert(esee != 0);

	int stack = 0;
	for (unsigned int i = currentline + 1; i < currentscript->lines.size(); i++) {
		token front = currentscript->lines[i].front();
		if (front.func == enu) {
			stack++;
		} else if (front.func == esee) {
			stack++;
		} else if (front.func == next) {
			if (stack) stack--;
			else {
				currentline = i;
				return;
			}
		}
	}
	currentline = currentscript->lines.size();
	std::cerr << "caosVM: couldn't find matching NEXT, stopping script\n";
}

void caosVM::jumpToNextIfBlock() {
	cmdinfo *doif = getCmdInfo("DOIF", true); assert(doif != 0);
	cmdinfo *elif = getCmdInfo("ELIF", true); assert(elif != 0);
	cmdinfo *els = getCmdInfo("ELSE", true); assert(els != 0);
	cmdinfo *endi = getCmdInfo("ENDI", true); assert(endi != 0);
	int stack = 0;
	
	for (unsigned int i = currentline + 1; i < currentscript->lines.size(); i++) {
		token front = currentscript->lines[i].front();
		if (front.func == doif) {
			stack++;
		} else if (front.func == endi) {
			if (stack) { stack--; continue; }
			currentline = i; return;
		} else if (stack) {
			continue;
		} else if ((front.func == elif) && !truthstack.back()) {
			currentline = i; return;
		} else if ((front.func == els) && !truthstack.back()) {
			currentline = i + 1; return; // we don't NEED to actually run 'else'
		}
	}
	currentline = currentscript->lines.size();
	std::cerr << "caosVM: couldn't find matching block for IF blocks, stopping script\n";
}

/**
 DOIF (command) condition (condition)
 
 Part of a DOIF/ELIF/ELSE/ENDI block. Jump to the next part of the block if condition is false, otherwise continue executing.
*/
void caosVM::c_DOIF() {
	VM_VERIFY_SIZE(0)
	truthstack.push_back(truth);
	if (!truth) jumpToNextIfBlock();
}

/**
 ELIF (command) condition (condition)
 
 Part of a DOIF/ELIF/ELSE/ENDI block. If none of the previous DOIF/ELIF conditions have been true, and condition evaluates to true, then the code in the ELIF block is executed.
*/
void caosVM::c_ELIF() {
	VM_VERIFY_SIZE(0)
	assert(!truthstack.empty());
	if (!truth || truthstack.back()) jumpToNextIfBlock();
	else { truthstack.pop_back(); truthstack.push_back(true); }
}

/**
 ELSE (command)
 
 Part of a DOIF/ELIF/ELSE/ENDI block. If ELSE is present, it is jumped to when none of the previous DOIF/ELIF conditions are true.
*/
void caosVM::c_ELSE() {
	VM_VERIFY_SIZE(0)
	assert(!truthstack.empty());
	assert(truthstack.back());
	jumpToNextIfBlock();
}

/**
 ENDI (command)
 
 The end of a DOIF/ELIF/ELSE/ENDI block.
*/
void caosVM::c_ENDI() {
	VM_VERIFY_SIZE(0)
	assert(!truthstack.empty());
	truthstack.pop_back();
}

/**
 REPS (command) reps (integer)
*/
void caosVM::c_REPS() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(reps)
	assert(reps > 0);
	repstack.push_back(reps);
	linestack.push_back(currentline + 1);
}

/**
 REPE (command)
*/
void caosVM::c_REPE() {
	VM_VERIFY_SIZE(0)
	assert(!linestack.empty());
	assert(!repstack.empty());
	int i = repstack.back() - 1;
	repstack.pop_back();
	if (i) {
		repstack.push_back(i);
		currentline = linestack.back();
	} else linestack.pop_back();
}

/**
 LOOP (command)
 
 The start of a LOOP..EVER or LOOP..UNTL loop.
*/
void caosVM::c_LOOP() {
	VM_VERIFY_SIZE(0)
	linestack.push_back(currentline + 1);
}

/**
 EVER (command)
 
 Jump back to the matching LOOP, no matter what.
*/
void caosVM::c_EVER() {
	VM_VERIFY_SIZE(0)
	assert(!linestack.empty());
	currentline = linestack.back();
}

/**
 UNTL (command) condition (condition)
 
 Jump back to the matching LOOP unless the condition evaluates to true.
*/
void caosVM::c_UNTL() {
	VM_VERIFY_SIZE(0)
	assert(!linestack.empty());
	if (!truth) currentline = linestack.back();
	else linestack.pop_back();
}

/**
 GSUB (command) label (label)
 
 Jump to a subroutine defined by SUBR with label 'label'.
*/
void caosVM::c_GSUB() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_STRING(label)
	cmdinfo *subr = getCmdInfo("SUBR", true); assert(subr != 0);
	for (unsigned int i = 0; i < currentscript->lines.size(); i++) {
		std::list<token>::iterator j = currentscript->lines[i].begin();
		// we're assuming that ++j is a label here, but the parser should force that issue
		if (((*j).func == subr) && ((*++j).data == label)) {
			linestack.push_back(currentline + 1);
			currentline = i + 1;
			return;
		}
	}
	std::cerr << "warning: GSUB didn't find matching SUBR for " << label << ", ignoring\n";
}

/**
 SUBR (command) label (label)
 
 Define the start of a subroute to be called with GSUB, with label 'label'.
 If the command is encountered during execution, it acts like a STOP.
*/
void caosVM::c_SUBR() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_STRING(label)
	c_STOP();
}

/**
 RETN (command)
 
 Return from a subroutine called with GSUB.
*/
void caosVM::c_RETN() {
	VM_VERIFY_SIZE(0)
	assert(!linestack.empty());
	currentline = linestack.back();
	linestack.pop_back();
}

/**
 NEXT (command)
*/
void caosVM::c_NEXT() {
	VM_VERIFY_SIZE(0)
	assert(!enumstack.empty());
	/* Remove any agents which disappeared mysteriously */
	while (!enumstack.back().empty() && !enumstack.back().back())
		enumstack.back().pop_back();
	/* If we're the first line in the enumeration block, or are out of
	 * agents to enumerate, pop off the stack and just return.
	 */
	if (currentline == linestack.back() || enumstack.back().empty()) {
		enumstack.pop_back();
		linestack.pop_back();
		setTarg(owner);
		return;
	}
	setTarg(enumstack.back().back());
	enumstack.back().pop_back();
	currentline = linestack.back();
}

/**
 ENUM (command) family (integer) genus (integer) species (integer)
*/
void caosVM::c_ENUM() {
	VM_VERIFY_SIZE(3)
	VM_PARAM_INTEGER(species) assert(species >= 0); assert(species <= 65535);
	VM_PARAM_INTEGER(genus) assert(genus >= 0); assert(genus <= 255);
	VM_PARAM_INTEGER(family) assert(family >= 0); assert(family <= 255);

	enumstack.push_back(std::vector<AgentRef>());
	linestack.push_back(currentline + 1); // loop entry
	
	for (std::multiset<Agent *, agentzorder>::iterator i
			= world.agents.begin(); i != world.agents.end(); i++) {
		Agent *a = (*i);
		if (species && species != a->species) continue;
		if (genus && genus != a->genus) continue;
		if (family && family != a->family) continue;

		enumstack.back().push_back(a);
	}

/*	std::cout << "ENUM " << (int) family << " " << (int) genus << " "
		<< (int) species << " found a total of " << enumstack.back().size()
		<< " agents." << std::endl; */

	jumpToEquivalentNext();
}

/**
 ESEE (command) family (integer) genus (integer) species (integer)
 
 like ENUM, but iterate through agents OWNR can see (todo: document exact rules)
*/
void caosVM::c_ESEE() {
	VM_VERIFY_SIZE(3)
	VM_PARAM_INTEGER(species) assert(species >= 0); assert(species <= 65535);
	VM_PARAM_INTEGER(genus) assert(genus >= 0); assert(genus <= 255);
	VM_PARAM_INTEGER(family) assert(family >= 0); assert(family <= 255);

	enumstack.push_back(std::vector<AgentRef>());
	linestack.push_back(currentline + 1); // loop entry
	
	for (std::multiset<Agent *, agentzorder>::iterator i
			= world.agents.begin(); i != world.agents.end(); i++) {
		Agent *a = (*i);
		if (species && species != a->species) continue;
		if (genus && genus != a->genus) continue;
		if (family && family != a->family) continue;

		// XXX: measure from center?
		double deltax = (*i)->x - owner->x;
		double deltay = (*i)->y - owner->y;
		deltax *= deltax;
		deltay *= deltay;

		double distance = sqrt(deltax + deltay);
		if (distance > owner->range) continue;

		enumstack.back().push_back(a);
	}

/*	std::cout << "ESEE " << (int) family << " " << (int) genus << " "
		<< (int) species << " found a total of " << enumstack.back().size()
		<< " agents." << std::endl; */

	jumpToEquivalentNext();
}

/**
 ETCH (command) family (integer) genus (integer) species (integer)

 like ENUM, but iterate through agents OWNR is touching
*/
void caosVM::c_ETCH() {
	VM_VERIFY_SIZE(3)
	VM_PARAM_INTEGER(species) assert(species >= 0); assert(species <= 65535);
	VM_PARAM_INTEGER(genus) assert(genus >= 0); assert(genus <= 255);
	VM_PARAM_INTEGER(family) assert(family >= 0); assert(family <= 255);

	// TODO: should probably implement this (ESEE)
	
	setTarg(owner);
	jumpToEquivalentNext();
	currentline++;
}

/**
 EPAS (command) family (integer) genus (integer) species (integer)

 like ENUM, but iterate through OWNR vehicle's passengers
*/
void caosVM::c_EPAS() {
	VM_VERIFY_SIZE(3)
	VM_PARAM_INTEGER(species) assert(species >= 0); assert(species <= 65535);
	VM_PARAM_INTEGER(genus) assert(genus >= 0); assert(genus <= 255);
	VM_PARAM_INTEGER(family) assert(family >= 0); assert(family <= 255);

	// TODO: should probably implement this (ESEE)
	
	setTarg(owner);
	jumpToEquivalentNext();
	currentline++;
}

