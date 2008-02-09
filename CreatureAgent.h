/*
 *  CreatureAgent.h
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

#include "Agent.h"
#include "genome.h"

class Creature;

class CreatureAgent : public Agent {
protected:
	Creature *creature;
	friend class Creature;

	unsigned int direction;
	bool walking, approaching;
	AgentRef approachtarget;

	virtual void creatureBorn() { }
	virtual void creatureAged() { }
	virtual void creatureDied() { }
	virtual void creatureInit() { }
	void setCreature(Creature *c);

public:
	CreatureAgent(unsigned char _family);
	virtual ~CreatureAgent();
	void tick();

	Creature *getCreature() { return creature; }
	
	void setDirection(unsigned int d) { assert(d < 4); direction = d; }
	unsigned int getDirection() { return direction; }
	void startWalking();
	void stopWalking();
	void approach(AgentRef it);
	bool isApproaching() { return approaching; }	
};

/* vim: set noet: */
