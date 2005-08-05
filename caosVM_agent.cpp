/*
 *  caosVM_agent.cpp
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
#include "openc2e.h"
#include "Vehicle.h"
#include "World.h"
#include "creaturesImage.h"
#include "SimpleAgent.h"
#include <iostream>
using std::cerr;

/**
 TOUC (integer) first (agent) second (agent)
*/
void caosVM::v_TOUC() {
	VM_VERIFY_SIZE(2)
	VM_PARAM_AGENT(second)
	VM_PARAM_AGENT(first)

	result.setInt(0);

	// I did this at 4:30am and have no idea if it works - fuzzie
	if (first->x < second->x) {
		if ((first->x + first->getWidth()) < second->x)
			return;
	} else {
		if ((second->x + second->getWidth()) < first->x)
			return;
	}

	if (first->y < second->y) {
		if ((first->y + first->getHeight()) < second->y)
			return;
	} else {
		if ((second->y + second->getHeight()) < first->y)
			return;
	}

	result.setInt(1);
}

/**
 RTAR (command) family (integer) genus (integer) species (integer)

 set targ to random agent with given family/genus/species
 */ 
void caosVM::c_RTAR() {
	VM_VERIFY_SIZE(3)
	VM_PARAM_INTEGER(species)
	VM_PARAM_INTEGER(genus)
	VM_PARAM_INTEGER(family)
	setTarg(0);

	/* XXX: maybe use a map of classifier -> agents? */
	std::vector<Agent *> temp;
	for (std::multiset<Agent *, agentzorder>::iterator i
		= world.agents.begin(); i != world.agents.end(); i++) {
		
		Agent *a = (*i);
		
		if (species && species != a->species) continue;
		if (genus && genus != a->genus) continue;
		if (family && family != a->family) continue;

		temp.push_back(a);
	}

	if (temp.size() == 0) return;
	int i = rand() % temp.size();
	setTarg(temp[i]);
}

/**
 NEW: SIMP (command) family (integer) genus (integer) species (integer) sprite_file (string) image_count (integer) first_image (integer) plane (integer)

 create a new simple agent with given family/genus/species, given spritefile with image_count sprites
 available starting at first_image in the spritefile, at the screen depth given by plane
*/
void caosVM::c_NEW_SIMP() {
	VM_VERIFY_SIZE(7)
	VM_PARAM_INTEGER(plane)
	VM_PARAM_INTEGER(first_image)
	VM_PARAM_INTEGER(image_count)
	VM_PARAM_STRING(sprite_file)
	VM_PARAM_INTEGER(species) caos_assert(species >= 0); caos_assert(species <= 65535);
	VM_PARAM_INTEGER(genus) caos_assert(genus >= 0); caos_assert(genus <= 255);
	VM_PARAM_INTEGER(family) caos_assert(family >= 0); caos_assert(family <= 255);

	SimpleAgent *a = new SimpleAgent(family, genus, species, plane, first_image, image_count);
	a->setImage(sprite_file);
	setTarg(a);
}

/**
 NEW: COMP (command) family (integer) genus (integer) species (integer) sprite_file (string) image_count (integer) first_image (integer) plane (integer)

 create a new composite agent with given family/genus/species, given spritefile with image_count sprites
 available starting at first_image in the spritefile, with the first part at the screen depth given by plane
*/
void caosVM::c_NEW_COMP() {
	VM_VERIFY_SIZE(7)
	VM_PARAM_INTEGER(plane)
	VM_PARAM_INTEGER(first_image)
	VM_PARAM_INTEGER(image_count)
	VM_PARAM_STRING(sprite_file)
	VM_PARAM_INTEGER(species) caos_assert(species >= 0); caos_assert(species <= 65535);
	VM_PARAM_INTEGER(genus) caos_assert(genus >= 0); caos_assert(genus <= 255);
	VM_PARAM_INTEGER(family) caos_assert(family >= 0); caos_assert(family <= 255);

	CompoundAgent *a = new CompoundAgent(family, genus, species, plane, sprite_file, first_image, image_count);
	setTarg(a);
}

/**
 NEW: VHCL (command) family (integer) genus (integer) species (integer) sprite_file (string) image_count (integer) first_image (integer) plane (integer)

 create a new vehicle agent with given family/genus/species, given spritefile with image_count sprites
 available starting at first_image in the spritefile, with the first part at the screen depth given by plane
*/
void caosVM::c_NEW_VHCL() {
	VM_VERIFY_SIZE(7)
	VM_PARAM_INTEGER(plane)
	VM_PARAM_INTEGER(first_image)
	VM_PARAM_INTEGER(image_count)
	VM_PARAM_STRING(sprite_file)
	VM_PARAM_INTEGER(species) caos_assert(species >= 0); caos_assert(species <= 65535);
	VM_PARAM_INTEGER(genus) caos_assert(genus >= 0); caos_assert(genus <= 255);
	VM_PARAM_INTEGER(family) caos_assert(family >= 0); caos_assert(family <= 255);

	Vehicle *a = new Vehicle(family, genus, species, plane, sprite_file, first_image, image_count);
	setTarg(a);
}

/**
 TARG (agent)

 return TARG
*/
void caosVM::v_TARG() {
	VM_VERIFY_SIZE(0)
	result.setAgent(targ);
}

/**
 OWNR (agent)
 
 return OWNR
*/
void caosVM::v_OWNR() {
	VM_VERIFY_SIZE(0)
	result.setAgent(owner);
}

/**
 NULL (agent)

 return null (zero) agent
*/
void caosVM::v_NULL() {
	const static AgentRef nullref;
	VM_VERIFY_SIZE(0)
	result.setAgent(nullref);
}

/**
 POSE (command) pose (integer)
*/
void caosVM::c_POSE() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(pose)

	caos_assert(targ);
	CompoundAgent *c = dynamic_cast<CompoundAgent *>(targ.get());
	if (c) {
		CompoundPart *p = c->part(part);
		p->setPose(pose);
	} else {
		SimpleAgent *a = dynamic_cast<SimpleAgent *>(targ.get());
		caos_assert(a);
		a->setPose(pose);
	}
}

/**
 ATTR (command) attr (integer)

 set attributes of TARG agent
*/
void caosVM::c_ATTR() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(attr)
	caos_assert(targ);
	targ->setAttributes(attr);
}

/**
 ATTR (integer)

 return attributes of TARG agent
*/
void caosVM::v_ATTR() {
	VM_VERIFY_SIZE(0)
	caos_assert(targ);
	result.setInt(targ->getAttributes());
}

/**
 TICK (command) tickrate (integer)
*/
void caosVM::c_TICK() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(tickrate)
	caos_assert(targ);
	targ->setTimerRate(tickrate);
}

/**
 BHVR (command) bhvr (integer)
*/
void caosVM::c_BHVR() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(bhvr)
	// TODO
}

/**
 TARG (command) agent (agent)

 set TARG to given agent
*/
void caosVM::c_TARG() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_AGENT(a)
	setTarg(a);
}

/**
 FROM (agent)
*/
void caosVM::v_FROM() {
	VM_VERIFY_SIZE(0)
	result.setAgent(0);
	// TODO
}

/**
 POSE (integer)
*/
void caosVM::v_POSE() {
	VM_VERIFY_SIZE(0)

	caos_assert(targ);

	CompoundAgent *n = dynamic_cast<CompoundAgent *>(targ.get());
	if (n) {
		CompoundPart *p = n->part(part);
		result.setInt(p->getPose());
		// TODO
	} else {
		SimpleAgent *n = dynamic_cast<SimpleAgent *>(targ.get());
		caos_assert(n);
		result.setInt(n->getPose());
	}
}

/**
 KILL (command) agent (agent)

 destroy the agent in question. you can't destroy PNTR.
 remember, use DEAD first for creatures!
*/
void caosVM::c_KILL() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_AGENT(a)
	if (a != world.hand())
		a->kill();
}

/**
 ANIM (command) poselist (byte-string)

 set the animation string for TARG, in the format '1 2 3 4'
 if it ends with '255', loop back to beginning; if it ends with '255 X', loop back to frame X

 <i>todo: compound agent stuff</i>
*/
void caosVM::c_ANIM() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_STRING(poselist)

	std::vector<unsigned int> *animation;

	caos_assert(targ);

	SimpleAgent *a;
 	CompoundAgent *c = dynamic_cast<CompoundAgent *>(targ.get());
	if (c) {
		animation = &c->part(part)->animation;
	} else {
		a = dynamic_cast<SimpleAgent *>(targ.get());
		caos_assert(a);
		animation = &a->animation;
	}
	
	animation->clear();

	std::string oh;
	for (unsigned int i = 0; i < poselist.size(); i++) {
		if (poselist[i] != ' ') {
			if (!isdigit(poselist[i])) throw badParamException();
			oh += poselist[i];
		} else {
			unsigned int j = (unsigned int)atoi(oh.c_str());
			animation->push_back(j);
			oh.clear();
		}
	}
	if (!oh.empty()) {
		unsigned int j = (unsigned int)atoi(oh.c_str());
		animation->push_back(j);
	}
	if (!animation->empty()) {
		if (c) c->part(part)->setFrameNo(0);
		else a->setFrameNo(0);
	}
}

/**
 ABBA (integer)
*/
void caosVM::v_ABBA() {
	VM_VERIFY_SIZE(0)
	// TODO
}

/**
 BASE (command) index (integer)
*/
void caosVM::c_BASE() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(index)
	// TODO
}

/**
 BASE (integer)
*/
void caosVM::v_BASE() {
	VM_VERIFY_SIZE(0)
	// TODO
}

/**
 BHVR (integer)
*/
void caosVM::v_BHVR() {
	VM_VERIFY_SIZE(0)

	result.setInt(0);
	// TODO
}

/**
 CARR (agent)
*/
void caosVM::v_CARR() {
	VM_VERIFY_SIZE(0)
	// TODO
	result.setAgent(0);
}

/**
 FMLY (integer)

 return family of TARG agent
*/
void caosVM::v_FMLY() {
	VM_VERIFY_SIZE(0)
	caos_assert(targ);
	result.setInt(targ->family);
}

/**
 GNUS (integer)

 return genus of TARG agent
*/
void caosVM::v_GNUS() {
	VM_VERIFY_SIZE(0)
	caos_assert(targ);
	result.setInt(targ->genus);
}

/**
 SPCS (integer)

 return species of TARG agent
*/
void caosVM::v_SPCS() {
	VM_VERIFY_SIZE(0)
	caos_assert(targ);
	result.setInt(targ->species);
}

/**
 PLNE (integer)

 return plane (z-order) of TARG agent
*/
void caosVM::v_PLNE() {
	VM_VERIFY_SIZE(0)
	caos_assert(targ);
	result.setInt(targ->getZOrder());
}

/**
 PNTR (agent)

 return the pointer agent (the hand)
*/
void caosVM::v_PNTR() {
	VM_VERIFY_SIZE(0)
	result.setAgent(world.hand());
}

unsigned int calculateScriptId(unsigned int message_id) {
	switch (message_id) {
		case 2: /* deactivate */
			return 0;
		case 0: /* activate 1 */
		case 1: /* activate 2 */
			return message_id + 1;
	}

	return message_id;
}

/**
 MESG WRIT (command) agent (agent) message_id (integer)
*/
void caosVM::c_MESG_WRIT() {
	VM_VERIFY_SIZE(2)
	VM_PARAM_INTEGER(message_id)
	VM_PARAM_AGENT(agent);

	caos_assert(agent);

	agent->fireScript(calculateScriptId(message_id));
}

/**
 MESG WRT+ (command) agent (agent) message_id (integer) param_1 (anything) param_2 (anything) delay (integer)
*/
void caosVM::c_MESG_WRT() {
	VM_VERIFY_SIZE(5)
	VM_PARAM_INTEGER(delay)
	// TODO: make into macros
	caosVar param_2 = params.back(); params.pop_back();
	caosVar param_1 = params.back(); params.pop_back();
	VM_PARAM_INTEGER(message_id)
	VM_PARAM_AGENT(agent)

	caos_assert(agent);

	// I'm not sure how to handle the 'delay'; is it a background delay, or do we actually block for delay ticks?
	
	agent->fireScript(calculateScriptId(message_id));
	agent->vm->setVariables(param_1, param_2);
	// TODO: set _p_ in agent's VM to param_1 and param_2
}

/**
 TOTL (integer) family (integer) genus (integer) species (integer)
*/
void caosVM::v_TOTL() {
	VM_VERIFY_SIZE(3)
	VM_PARAM_INTEGER(species) caos_assert(species >= 0); caos_assert(species <= 65535);
	VM_PARAM_INTEGER(genus) caos_assert(genus >= 0); caos_assert(genus <= 255);
	VM_PARAM_INTEGER(family) caos_assert(family >= 0); caos_assert(family <= 255);

	unsigned int x = 0;
	for (std::multiset<Agent *, agentzorder>::iterator i = world.agents.begin(); i != world.agents.end(); i++) {
		if ((*i)->family == family)
			if ((*i)->genus == genus)
				if ((*i)->species == species)
					x++;
	}
	result.setInt(x);
}

/**
 SHOW (command) visibility (integer)
 
 set visibility of agent to cameras. 0 = invisible, 1 = visible.
*/
void caosVM::c_SHOW() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(visibility)
	caos_assert((visibility == 0) || (visibility == 1));
	caos_assert(targ);
	targ->visible = visibility;
}

/**
 POSX (float)
*/
void caosVM::v_POSX() {
	VM_VERIFY_SIZE(0)
	caos_assert(targ);
	result.setFloat(targ->x + (targ->getWidth() / 2));
}

/**
 POSY (float)
*/
void caosVM::v_POSY() {
	VM_VERIFY_SIZE(0)
	caos_assert(targ);
	result.setFloat(targ->y + (targ->getHeight() / 2));
}

/**
 FRAT (command) framerate (integer)
*/
void caosVM::c_FRAT() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(framerate)

	caos_assert(targ);

	CompoundAgent *c = dynamic_cast<CompoundAgent *>(targ.get());
	if (c) {
		CompoundPart *p = c->part(part);
		p->setFramerate(framerate);
	} else {
		SimpleAgent *a = dynamic_cast<SimpleAgent *>(targ.get());
		caos_assert(a);
		a->setFramerate(framerate);
	}
}

/**
 OVER (command)

 wait until the animation of the target agent or part is over
*/
void caosVM::c_OVER() {
	VM_VERIFY_SIZE(0)

	caos_assert(targ);

	int fno, animsize;
	
 	CompoundAgent *c = dynamic_cast<CompoundAgent *>(targ.get());
	if (c) {
		CompoundPart *p = c->part(part);
		fno = p->getFrameNo();
		animsize = p->animation.size();
	} else {
		SimpleAgent *a = dynamic_cast<SimpleAgent *>(targ.get());
		caos_assert(a);
		fno = a->getFrameNo();
		animsize = a->animation.size();
	}

	if (fno + 1 == animsize) blocking = false;
	else if (animsize == 0) blocking = false;
	else blocking = true;
}

/**
 PUHL (command) pose (integer) x (integer) y (integer)

 set relative x/y coords for TARG's pickup point
 pose is -1 for all poses, or a pose relative to the base specified in NEW: (not BASE)
*/
void caosVM::c_PUHL() {
	VM_VERIFY_SIZE(3)
	VM_PARAM_INTEGER(y)
	VM_PARAM_INTEGER(x)
	VM_PARAM_INTEGER(pose)

	caos_assert(targ);
	// TODO
}

/**
 POSL (float)
*/
void caosVM::v_POSL() {
	VM_VERIFY_SIZE(0)

	caos_assert(targ);
	result.setFloat(targ->x);
}

/**
 POST (float)
*/
void caosVM::v_POST() {
	VM_VERIFY_SIZE(0)

	caos_assert(targ);
	result.setFloat(targ->y);
}

/**
 POSR (float)
*/
void caosVM::v_POSR() {
	VM_VERIFY_SIZE(0)

	caos_assert(targ);
	result.setFloat(targ->x + targ->getWidth());
}

/**
 POSB (float)
*/
void caosVM::v_POSB() {
	VM_VERIFY_SIZE(0)

	caos_assert(targ);
	result.setFloat(targ->y + targ->getHeight());
}

/**
 WDTH (integer)
*/
void caosVM::v_WDTH() {
	VM_VERIFY_SIZE(0)

	caos_assert(targ);
	result.setInt(targ->getWidth());
}

/**
 PLNE (command) depth (integer)
*/
void caosVM::c_PLNE() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(depth)

	caos_assert(targ);
	targ->setZOrder(depth);
}

/**
 TINT (command) red_tint (integer) green_tint (integer) blue_tint (integer) rotation (integer) swap (integer)
*/
void caosVM::c_TINT() {
	VM_VERIFY_SIZE(5)
	VM_PARAM_INTEGER(swap)
	VM_PARAM_INTEGER(rotation)
	VM_PARAM_INTEGER(blue_tint)
	VM_PARAM_INTEGER(green_tint)
	VM_PARAM_INTEGER(red_tint)

	caos_assert(targ);

	// TODO
}

/**
 RNGE (command) distance (float)
*/
void caosVM::c_RNGE() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_FLOAT(distance)

	caos_assert(targ)
	targ->range = distance;
}

/**
 RNGE (integer)
*/
void caosVM::v_RNGE() {
	VM_VERIFY_SIZE(0)
	caos_assert(targ);
	result.setFloat(targ->range);
}

/**
 TRAN (integer) x (integer) y (integer)

 Tests if the pixel at (x,y) on TARG is transparent
*/
void caosVM::v_TRAN() {
	VM_VERIFY_SIZE(2)
	VM_PARAM_INTEGER(y)
	VM_PARAM_INTEGER(x)

	caos_assert(targ);
	SimpleAgent *a = dynamic_cast<SimpleAgent *>(targ.get());
	if (!a) {
		// TODO: TRAN on other agents
		// (if lc2e even allows that)
		// it seems to work for compound agents - fuzzie
		result.setInt(0);
		return;
	}

	creaturesImage *i = a->getSprite();
	int index = a->getCurrentSprite();

	unsigned char *data = (unsigned char *)i->data(index);
	// XXX: do we measure from center?
	int w = i->width(index);
	int h = i->height(index);

	caos_assert(x < w);
	caos_assert(x >= 0);
	caos_assert(y < h);
	caos_assert(y >= 0);
	
	if (data[w * y + x] == 0)
		result.setInt(1);
	else
		result.setInt(0);
}
	
/**
 TRAN (command) transparency (integer) part_no (integer)
*/
void caosVM::c_TRAN() {
	VM_VERIFY_SIZE(2)
	VM_PARAM_INTEGER(part_no)
	VM_PARAM_INTEGER(transparency)

	caos_assert(targ);
	// TODO
}

/**
 HGHT (integer)
*/
void caosVM::v_HGHT() {
	VM_VERIFY_SIZE(0)
	caos_assert(targ);

	result.setInt(targ->getHeight());
}

/**
 HAND (string)

 returns the name of the hand; default is 'hand'
*/
void caosVM::v_HAND() {
	VM_VERIFY_SIZE(0)

	result.setString(world.handName);
}

/**
 HAND (command) name (string)

 set the name of the hand
*/
void caosVM::c_HAND() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_STRING(name)

	world.handName = name;
}

