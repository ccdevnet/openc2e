/*
 *  caosVM_world.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Sun Aug 28 2005.
 *  Copyright (c) 2005 Alyssa Milburn. All rights reserved.
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

/**
 LOAD (command) worldname (string)
 %status stub
*/
void caosVM::c_LOAD() {
	VM_PARAM_STRING(worldname)

	// TODO
}

/**
 SAVE (command)
 %status stub

 Save the world at the start of the next tick. Beware; if you don't put this
 in an INST, it might save directly after your SAVE call (meaning upon loading,
 the script will execute the next instruction, often QUIT or LOAD, which is
 bad).
*/
void caosVM::c_SAVE() {
	// note, world.saving doesn't mean anything yet
	world.saving = true;
}

/**
 QUIT (command)
 %status maybe

 Quit the game engine at the start of the nexttick
*/
void caosVM::c_QUIT() {
	// TODO
	world.quitting = true;
}

/**
 WNAM (string)
 %status stub

 Returns the name of the current world.
*/
void caosVM::v_WNAM() {
	// result.setString(world.name);
	result.setString("oh"); // TODO
}

/**
 WUID (string)
 %status stub

 Returns the unique identifier (moniker?) of the current world.
*/
void caosVM::v_WUID() {
	// result.setString(world.moniker);
	result.setString("dock-aaaaa-bbbbb-ccccc-ddddd"); // TODO
}

/**
 WTNT (command) index (integer) red (integer) green (integer) blue (integer) rotation (integer) swap (integer)
 %status stub

 Sets an index in the mysterious global tint table to have the specified values. No, we have no idea what that means either.
*/
void caosVM::c_WTNT() {
	VM_PARAM_INTEGER(swap)
	VM_PARAM_INTEGER(rotation)
	VM_PARAM_INTEGER(blue)
	VM_PARAM_INTEGER(green)
	VM_PARAM_INTEGER(red)
	VM_PARAM_INTEGER(index)

	// TODO
}

/**
 NWLD (integer)
 %status stub
*/
void caosVM::v_NWLD() {
	result.setInt(0); // TODO
}

/**
 WRLD (command) name (string)
 %status stub

 Create a new world directory to prepare for the creation of the specified world.
*/
void caosVM::c_WRLD() {
	VM_PARAM_STRING(name)

	// TODO
}

/**
 WRLD (string) world (integer)
 %status stub

 Return the name of the specified world (zero-indexed, see NWLD).
*/
void caosVM::v_WRLD() {
	VM_PARAM_INTEGER(world)

	caos_assert(false); // TODO
}

/**
 PSWD (command) password (string)
 %status stub
*/
void caosVM::c_PSWD() {
	VM_PARAM_STRING(password)

	// TODO
}

/**
 PSWD (string) world (integer)
 %status stub

 Return the password for the specified world (zero-indexed, see NWLD), or an empty string for no password.
*/
void caosVM::v_PSWD() {
	VM_PARAM_INTEGER(world)

	result.setString(""); // TODO
}

/**
 WNTI (integer) name (string)
 %status stub

 Return the world identifier for the specified world name, or -1 if it doesn't exist.
*/
void caosVM::v_WNTI() {
	VM_PARAM_STRING(name)

	result.setInt(-1); // TODO
}

/**
 DELW (command) name (string)
 %status stub

 Delete the specified world directory and all contents.
*/
void caosVM::c_DELW() {
	VM_PARAM_STRING(name)

	caos_assert(false); // TODO
}

/* vim: set noet: */
