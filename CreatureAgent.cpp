/*
 *  CreatureAgent.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Sat Dec 09 2006.
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

#include "CreatureAgent.h"
#include "Creature.h"

CreatureAgent::CreatureAgent(unsigned char _family) : Agent(_family, 0, 0, 0) {
	creature = 0;

	// TODO: set zorder randomly :) should be somewhere between 1000-2700, at a /guess/
	zorder = 1500;

	walking = false;
	approaching = false;
	direction = 0;
}

CreatureAgent::~CreatureAgent() {
	if (creature)
		delete creature;
}

void CreatureAgent::setCreature(Creature *c) {
	assert(c);
	creature = c;

	slots[0] = creature->getGenome();
	species = (creature->isFemale() ? 2 : 1);

	// category needs to be set, so call setClassifier now
	setClassifier(family, genus, species);
}

void CreatureAgent::tick() {
	Agent::tick();

	if (!paused)
		creature->tick();
}

void CreatureAgent::startWalking() {
	walking = true;
	approaching = false;
}

void CreatureAgent::stopWalking() {
	walking = false;
}

void CreatureAgent::approach(AgentRef it) {
	assert(it);

	walking = false;
	approaching = true;
	approachtarget = it;
}

/* vim: set noet: */
