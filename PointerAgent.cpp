/*
 *  PointerAgent.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Tue Aug 30 2005.
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

#include "PointerAgent.h"
#include "openc2e.h"
#include "World.h"
#include "Engine.h"
#include "caosVM.h"

// TODO: change imagecount?
PointerAgent::PointerAgent(std::string spritefile) : SimpleAgent(2, 1, 1, INT_MAX, spritefile, 0, 0) {
	name = "hand";
	handle_events = true;
	// TODO: verify attributes on the pointer in c2e
	attr.setInt(256); // camera shy
}

void PointerAgent::finishInit() {
	Agent::finishInit();

	// float relative to main camera
	attr.setInt(attr.getInt() & 32);
	floatSetup();
}

// TODO: this should have a queueScript equiv too
void PointerAgent::firePointerScript(unsigned short event, Agent *src) {
	assert(src); // TODO: I /think/ this should only be called by the engine..
	shared_ptr<script> s = src->findScript(event);
	if (!s && engine.version < 3) { // TODO: are we sure this doesn't apply to c2e?
		s = findScript(event); // TODO: we should make sure this actually belongs to the pointer agent and isn't a fallback, maybe
	}
	if (!s) return;
	if (!vm) vm = world.getVM(this);
	if (vm->fireScript(s, false, src)) { // TODO: should FROM be src?
		vm->setTarg(this);
		zotstack();
	}
}

void PointerAgent::physicsTick() {
	// TODO: this is a hack, which does nothing, because we set a velocity in main() but also move the cursor manually
}

void PointerAgent::kill() {
	// pointer agent isn't killable
}

/* vim: set noet: */
