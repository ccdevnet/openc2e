/*
 *  caosVM_scripts.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Tue Jun 01 2004.
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
using std::cerr;

/**
 INST (command)

 don't release control to the scheduler until end of script or SLOW
 */
void caosVM::c_INST() {
	VM_VERIFY_SIZE(0)
	cerr << "unimplemented: INST\n";
}

/**
 SLOW (command)

 reverts effects of INST
 */
void caosVM::c_SLOW() {
	VM_VERIFY_SIZE(0)
	cerr << "unimplemented: SLOW\n";
}

/**
 LOCK (command)

 prevent agent script from being interrupted by another
 */
void caosVM::c_LOCK() {
	VM_VERIFY_SIZE(0)
	cerr << "unimplemented: LOCK\n";
}

/**
 UNLK (command)

 reverts effects of LOCK
 */
void caosVM::c_UNLK() {
	VM_VERIFY_SIZE(0)
	cerr << "unimplemented: UNLK\n";
}

/**
 WAIT (command) ticks (integer)

 stop the script from running for a number of ticks
 */
void caosVM::c_WAIT() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(ticks)
	cerr << "unimplemented: WAIT\n";
}

/**
 STOP (command)
*/
void caosVM::c_STOP() {
	VM_VERIFY_SIZE(0)
	currentline = currentscript->lines.size();
}

/**
 SCRX (command) event (integer) family (integer) genus (integer) species (integer)

 delete the event script in question
*/
void caosVM::c_SCRX() {
	VM_VERIFY_SIZE(4)
	VM_PARAM_INTEGER(species) assert(species >= 0); assert(species <= 255);
	VM_PARAM_INTEGER(genus) assert(genus >= 0); assert(genus <= 255);
	VM_PARAM_INTEGER(family) assert(family >= 0); assert(family <= 65535);
	VM_PARAM_INTEGER(event) assert(event >= 0); assert(event <= 255);
	cerr << "unimplemented: SCRX\n";
}
