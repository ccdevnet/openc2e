/*
 *  Agent.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Tue May 25 2004.
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

#include "Agent.h"
#include "World.h"
#include "physics.h"
#include <iostream>
#include <sstream>
#include "caosVM.h"

Agent::Agent(unsigned char f, unsigned char g, unsigned short s, unsigned int p) :
  visible(true), family(f), genus(g), species(s), zorder(p), vm(0), timerrate(0) {
	velx.setFloat(0.0f);
	vely.setFloat(0.0f);
	accg = 0.3f;
	range = 500;
	sufferphysics = false;
	x = 0.0f; y = 0.0f;
	clac[0] = 1; // activate 1
	clik = -1;
	
	// AgentRef stuff
	self.ref = this;
	self.next = self.prev = &self;
	
	dying = false;
	unid = -1;

	zorder_iter = world.agents.insert(this);
}

Agent::~Agent() {
	world.agents.erase(zorder_iter);
	if (vm)
		world.freeVM(vm);
	zotrefs();
	if (unid != -1)
		world.freeUNID(unid);
}

void Agent::moveTo(float _x, float _y) {
	x = _x; y = _y;
}

void Agent::fireScript(unsigned short event) {
	if (dying) return;

	script &s = world.scriptorium.getScript(family, genus, species, event);
	if (s.lines.empty()) return;
	if (!vm) vm = world.getVM(this);
	if (vm->fireScript(s, (event == 9)))
		vm->setTarg(this);
	
	// This slows us down too much :)
#if 0
	std::cout << "Agent::fireScript fired " << (unsigned int)family << " " << (unsigned int)genus << " " << species << " ";
	const std::string n = world.catalogue.getAgentName(family, genus, species);
	if (n.size())
		std::cout << "(" << n << ") ";
	std::cout << (unsigned int)event << std::endl;
#endif
}

void Agent::tick() {
	if (dying) return;

	if (sufferphysics && accg) {
		float newvely = vely.floatValue + accg;
		float destx = x + velx.floatValue;
		float desty = y + newvely;
		//std::cout << x << ", " << y << ": " << destx << ", " << desty << "! " << accg << "\n";
		Room *r1 = world.map.roomAt((unsigned int)x, (unsigned int)y);
		if (!r1) {
			std::cout << "not doing physics on agent, outside room system!\n";
		}
		Room *r2 = world.map.roomAt((unsigned int)destx + getWidth(), (unsigned int)desty + getHeight());
		Room *r3 = world.map.roomAt((unsigned int)destx, (unsigned int)desty);

		if (r1 && (r1 == r2) && (r2 == r3)) {
			vely.setFloat(newvely);
			moveTo(destx, desty);
		} else if (r1) {
			physicsHandler p;

			//std::cout << x + getWidth() / 2 << " " << y + getHeight() << " " << destx + (getWidth() / 2) << " " << desty + getHeight() << " " << r1->x_left << " " << r1->y_left_floor << " " << r1->x_right << " " << r1->y_right_floor << "\n";

			bool c = p.collidePoints(x + (getWidth() / 2), y + getHeight(), destx + (getWidth() / 2), desty + getHeight(),
					r1->x_left, r1->y_left_floor, r1->x_right, r1->y_right_floor);
		
			bool hitfloor = false;
			if (c) {
				destx = p.getCollisionX() - (getWidth() / 2);
				desty = p.getCollisionY() - getHeight();
				if (desty != y) hitfloor = true;
			}

			r1 = world.map.roomAt(destx, desty);
			r2 = world.map.roomAt(destx + getWidth() - 1, desty + getHeight() - 1);

			if (r1 && r2) {
				// quick hack to see if we can travel to the next room
				bool hacked = false;
				if (r1 != r2) {
					if (r1->doors[r2] && r1->doors[r2]->perm == 100) {
						moveTo(destx, desty + 2);
						vely.setFloat(newvely);
						hacked = true;
					}
				}
				if (!hacked) {
					// okay, we can't travel to the next room.
					moveTo(destx, desty);
					// this is supposed to only fire if we hit the floor, but it's sort of iffy
					vely.setFloat(newvely);
					if (hitfloor) {
						fireScript(6); // script Collision
						if (vm) vm->setVariables(velx, vely);
					}
					// then set vely to 0 so we don't do stupid things
					vely.setFloat(0);
				}
			} else {
				// we're not in the room system..
				vely.setFloat(0);
			}
		}
	} else {
		if (vely.hasDecimal())
			y = y + vely.getFloat();
		if (velx.hasDecimal())
			x = x + velx.getFloat();
	}

	if (timerrate) {
		tickssincelasttimer++;
		if (tickssincelasttimer == timerrate) {
			fireScript(9);
			tickssincelasttimer = 0;
		}
	}

	if (vm)
		vm->tick();
}

void Agent::kill() {
	assert(!dying);
	dying = true; // what a world, what a world...
	if (vm)
		vm->stop();
	zotrefs();
	world.killqueue.push_back(this);
}

void Agent::zotrefs() {
	while (self.next != &self)
		self.next->clear();
}

void Agent::setZOrder(unsigned int z) {
	zorder = z;
	world.agents.erase(zorder_iter);
	zorder_iter = world.agents.insert(this);
}

int Agent::getUNID() {
      if (unid != -1)
              return unid;
      return unid = world.getUNID(this);
}

std::string Agent::identify() const {
	std::ostringstream o;
	o << (int)family << " " << (int)genus << " " << species << " ";
	const std::string n = world.catalogue.getAgentName(family, genus, species);
	if (n.size())
		o << "(" + n + ")";
	if (unid != -1)
		o << " unid " << unid;
	else
		o << " (no unid assigned)";
	return o.str();
}

bool agentzorder::operator ()(const Agent *s1, const Agent *s2) const {
	return s1->zorder < s2->zorder;
}

