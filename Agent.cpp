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

Agent::Agent(unsigned char f, unsigned char g, unsigned short s, unsigned int p) :
  visible(true), family(f), genus(g), species(s), zorder(p), vm(this), timerrate(0) {
  velx.setFloat(0.0f);
  vely.setFloat(0.0f);
  accg = 0.3f;
  sufferphysics = false;
}

void Agent::moveTo(float _x, float _y) {
	x = _x; y = _y;
}

void Agent::fireScript(unsigned short event) {
	script &s = world.scriptorium.getScript(family, genus, species, event);
	if (s.lines.empty()) return;
	vm.fireScript(s, (event == 9));
	vm.setTarg(this);
	
	std::cout << "Agent::fireScript fired " << (unsigned int)family << " " << (unsigned int)genus << " " << species << " ";
	const std::string n = world.catalogue.getAgentName(family, genus, species);
	if (n.size())
		std::cout << "(" << n << ") ";
	std::cout << (unsigned int)event << std::endl;
}

void Agent::tick() {
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
			
			if (c) {
				destx = p.getCollisionX() - (getWidth() / 2);
				desty = p.getCollisionY() - getHeight();
			}

			r1 = world.map.roomAt(destx, desty);
			r2 = world.map.roomAt(destx + getWidth(), desty + getHeight());

			if ((r1 && r2)) {
				moveTo(destx, desty);
				vely.setFloat(newvely);
			} else
				vely.setFloat(0);
		}
	// TODO: check velx!
	} else if (vely.hasFloat()) {
		y = y + vely.floatValue;
	} else if (vely.hasInt()) {
		y = y + vely.intValue;
	}

	if (timerrate) {
		tickssincelasttimer++;
		if (tickssincelasttimer == timerrate) {
			fireScript(9);
			tickssincelasttimer = 0;
		}
	}

	vm.tick();
}

