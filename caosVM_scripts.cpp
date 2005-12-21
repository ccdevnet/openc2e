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
#include "World.h"
#include <iostream>
using std::cerr;

/**
 INST (command)
 %status maybe

 don't release control to the scheduler until end of script or SLOW
 */
void caosVM::c_INST() {
	VM_VERIFY_SIZE(0)
	inst = true;
	// TODO: do we need a state similar to locked? i commented it out because it doesn't seem right - fuzzie
	//locked = true;
}

/**
 SLOW (command)
 %status maybe

 reverts effects of INST
 */
void caosVM::c_SLOW() {
	VM_VERIFY_SIZE(0)
	
	inst = false;
}

/**
 LOCK (command)
 %status maybe

 prevent agent script from being interrupted by another
 */
void caosVM::c_LOCK() {
	VM_VERIFY_SIZE(0)
	lock = true;
}

/**
 UNLK (command)
 %status maybe

 reverts effects of LOCK
 */
void caosVM::c_UNLK() {
	VM_VERIFY_SIZE(0)
	
	lock = false;
}

class blockUntilTime : public blockCond {
	protected:
		unsigned int end;
	public:
		bool operator()() {
			if (world.tickcount < end)
				return true;
			return false;
		}

		blockUntilTime(int delta) : end(world.tickcount + delta) {}
};

/**
 WAIT (command) ticks (integer)
 %status maybe

 stop the script from running for a number of ticks
 */
void caosVM::c_WAIT() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(ticks)

	caos_assert(ticks > 0); // todo: is this right?
	startBlocking(new blockUntilTime(ticks));
}

/**
 STOP (command)
 %status maybe
 %pragma retc -1
*/
void caosVM::c_STOP() {
	VM_VERIFY_SIZE(0)
	stop();
}

/**
 SCRX (command) family (integer) genus (integer) species (integer) event (integer)
 %status maybe

 delete the event script in question
*/
void caosVM::c_SCRX() {
	VM_VERIFY_SIZE(4)
	VM_PARAM_INTEGER(event) 
	caos_assert(event >= 0); 
	caos_assert(event <= 255);
	VM_PARAM_INTEGER(species) 
	caos_assert(species >= 0); 
	caos_assert(species <= 65535);
	VM_PARAM_INTEGER(genus) 
	caos_assert(genus >= 0); 
	caos_assert(genus <= 255);
	VM_PARAM_INTEGER(family) 
	caos_assert(family >= 0); 
	caos_assert(family <= 255);
	world.scriptorium.delScript(family, genus, species, event);
}

/**
 CODE (integer)

 Return script number running in the target. Returns -1 if target is not
 running anything (or if it's running something that's not an event script)
*/
void caosVM::v_CODE() {
	caos_assert(targ);
	int res;
	if (targ->vm && targ->vm->currentscript)
		res = targ->vm->currentscript->scrp;
	else
		res = -1;
	
	result.setInt(res);
}

/**
 CODF (integer)

 Return script family running in the target. Returns -1 if target is not
 running anything (or if it's running something that's not an event script)
*/
void caosVM::v_CODF() {
	caos_assert(targ);
	int res;
	if (targ->vm && targ->vm->currentscript)
		res = targ->vm->currentscript->fmly;
	else
		res = -1;
	
	result.setInt(res);
}

/**
 CODG (integer)

 Return script genus running in the target. Returns -1 if target is not
 running anything (or if it's running something that's not an event script)
*/
void caosVM::v_CODG() {
	caos_assert(targ);
	int res;
	if (targ->vm && targ->vm->currentscript)
		res = targ->vm->currentscript->gnus;
	else
		res = -1;
	
	result.setInt(res);
}

/**
 CODS (integer)

 Return script species running in the target. Returns -1 if target is not
 running anything (or if it's running something that's not an event script)
*/
void caosVM::v_CODS() {
	caos_assert(targ);
	int res;
	if (targ->vm && targ->vm->currentscript)
		res = targ->vm->currentscript->spcs;
	else
		res = -1;
	
	result.setInt(res);
}
/* vim: set noet: */
