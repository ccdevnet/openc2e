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
#include "MetaRoom.h"
#include "World.h"
#include "Engine.h"
#include <iostream>
#include "caosVM.h"
#include "AudioBackend.h"
#include <boost/format.hpp>
#include "Room.h"
#include "Vehicle.h"
#include "AgentHelpers.h"
#include "creaturesImage.h"

void Agent::core_init() {
	initialized = false;
	lifecount = 0;
}

Agent::Agent(unsigned char f, unsigned char g, unsigned short s, unsigned int p) :
  vm(0), zorder(p), timerrate(0), visible(true) {
	core_init();

	setClassifier(f, g, s);

	lastScript = -1;
	
	velx.setFloat(0.0f);
	vely.setFloat(0.0f);

	wasmoved = false;

	if (engine.version == 2) {
		accg = 10;
		aero = 20;
		rest = 40;
		
		size = 127; // TODO: correct default?
		thrt = 0;
		// TODO: it looks like grav should be 0, but make sure!
	} else if (engine.version > 2) {
		accg = 0.3f;
		aero = 0;
		elas = 0;

		perm = 50; // TODO: correct default?
		falling = true;
	}
	
	range = 500;

	// TODO: is this the correct default?
	clac[0] = 0; // message# for activate 1
	if (engine.version < 3) {
		// TODO: is this the correct default? (this is equivalent to bhvr click == 0)
		clac[0] = -1; clac[1] = -1; clac[2] = -1;
	}
	clik = -1;
	
	dying = false;
	unid = -1;

	paused = displaycore = false;

	cr_can_push = cr_can_pull = cr_can_stop = cr_can_hit = cr_can_eat = cr_can_pickup = false; // TODO: check this
	imsk_key_down = imsk_key_up = imsk_mouse_move = imsk_mouse_down = imsk_mouse_up = imsk_mouse_wheel = imsk_translated_char = false;

	emitca_index = -1; emitca_amount = 0.0f;

	objp.setAgent(0); // not strictly necessary
}

void Agent::finishInit() {
	// lc2e, at least, seems to position agents centered on (-9876,-9876) to begin with
	// TODO: where should we place agents in other games? is this code right at all anyway?
	// (bear in mind that there are no parts present for some C1/C2 agents when finishInit is called, atm)
	if (engine.version > 2) {
		x = -9876.0f + (getWidth() / 2.0f); y = -9876.0f + (getHeight() / 2.0f);
	}
	
	// shared_from_this() can only be used if these is at least one extant
	// shared_ptr which owns this
	world.agents.push_front(boost::shared_ptr<Agent>(this));
	agents_iter = world.agents.begin();

	if (findScript(10))
		queueScript(10); // constructor
	
	initialized = true;
}

void Agent::zotstack() {
	// Zap the VM stack.
	for (std::list<caosVM *>::iterator i = vmstack.begin(); i != vmstack.end(); i++) {
		world.freeVM(*i);
	}
	vmstack.clear();
}

void Agent::moveTo(float _x, float _y, bool force) {
	// Move ourselves to the specified location.
	wasmoved = true;

	// if we're being carried and aren't being forced to move (prbly by our carrier), forget it
	if (carriedby && !force) return;
	
	// TODO: what if we move while being carried? doomy explosions ensue, that's what!
	float xoffset = _x - x;
	float yoffset = _y - y;		

	x = _x; y = _y;

	// handle wraparound
	// TODO: this is perhaps non-ideal
	if (engine.version < 3 && xoffset != 0.0f) {
		// TODO: it'd be nice to handle multiple metarooms
		MetaRoom *m = world.map.getFallbackMetaroom();
		assert(m);

		if (x < m->x()) {
			x += m->width();
		} else if (x > m->x() + m->width()) {
			x -= m->width();
		}
	}

	for (std::vector<AgentRef>::iterator i = floated.begin(); i != floated.end(); i++) {
		assert(*i);
		(*i)->moveTo((*i)->x + xoffset, (*i)->y + yoffset);
	}

	adjustCarried(xoffset, yoffset);
}

void Agent::floatTo(AgentRef a) {
	std::vector<AgentRef>::iterator i = std::find(floated.begin(), floated.end(), a);
	assert(i == floated.end()); // loops are bad, mmkay

	if (floatable()) floatRelease();
	floatingagent = a;
	if (floatable()) floatSetup();
}

void Agent::floatTo(float x, float y) {
	if (floatingagent) {
		moveTo(floatingagent->x + x, floatingagent->y + y);
	} else {
		moveTo(world.camera.getX() + x, world.camera.getY() + y);
	}
}

void Agent::floatSetup() {
	if (floatingagent)
		floatingagent->addFloated(this);
	else
		world.camera.addFloated(this);
}

void Agent::floatRelease() {
	if (floatingagent) {
		floatingagent->delFloated(this);
	} else
		world.camera.delFloated(this);
}

void Agent::addFloated(AgentRef a) {
	assert(a);
	assert(a != floatingagent); // loops are bad, mmkay
	floated.push_back(a);
}

void Agent::delFloated(AgentRef a) {
	assert(a);
	std::vector<AgentRef>::iterator i = std::find(floated.begin(), floated.end(), a);
	//if (i == floated.end()) return;
	assert(i != floated.end());
	floated.erase(i);
}

shared_ptr<script> Agent::findScript(unsigned short event) {
	return world.scriptorium.getScript(family, genus, species, event);
}

#include "PointerAgent.h"
#include "CreatureAgent.h"
#include "Creature.h"
bool Agent::fireScript(unsigned short event, Agent *from, caosVar one, caosVar two) {
	// Start running the specified script on the VM of this agent, with FROM set to the provided agent.

	if (dying) return false;

	CreatureAgent *c = 0;
	if (event <= 3 || event == 4 || event == 12 || event == 13 || event == 14)
		c = dynamic_cast<CreatureAgent *>(from);

	switch (event) {
		case 0: // deactivate
			if (c && !cr_can_stop) return false;
			// TODO: not sure if this is the right place to do this.
			actv.setInt(event);
			break;
		case 1: // activate 1
			if (c && !cr_can_push) return false;
			// TODO: not sure if this is the right place to do this.
			actv.setInt(event);
			break;
		case 2: // activate 2
			if (c && !cr_can_pull) return false;
			// TODO: not sure if this is the right place to do this.
			actv.setInt(event);
			break;
		case 3: // hit
			if (c && !cr_can_hit) return false;
			break;
		case 4: // pickup
			if (c && !cr_can_pickup) return false;
			if (!from) return false;
			if (from == world.hand()) {
				if (!mouseable()) return false;
			} else if (!c) {
				// TODO: valid check for vehicles?
				if (!carryable()) return false;
			}
			from->addCarried(this); // TODO: correct behaviour?
			break;
		case 5: // drop
			if (!from) return false;
			// TODO: this check isn't very good for vehicles ;p
			// if (from != carriedby) return false;
			break;
		case 12: // eat
			if (c && !cr_can_eat) return false;
			break;
		case 13: // hold hands with pointer
			if (c) {
				// TODO
			}
			break;
		case 14: // stop holding hands with pointer
			if (c) {
				// TODO
			}
			break;
		case 92: // TODO: hack for 'UI Mouse Down' event - we need a real event system!
			std::cout << "faking event 92 on " << identify() << std::endl;
			CompoundPart *p = world.partAt(world.hand()->x, world.hand()->y);
			if (!p || p->getParent() != this) // if something is horridly broken here, return
				return false; // was caos_assert(p && p->getParent() == this);
			p->handleClick(world.hand()->x - p->x - p->getParent()->x, world.hand()->y - p->y - p->getParent()->y);
			// TODO: we're [obviously] missing firing the pointer script here, but it's a hack for now
			break;
	}

	bool ranscript = false;

	shared_ptr<script> s = findScript(event);
	if (s) {
		bool madevm = false;
		if (!vm) { madevm = true; vm = world.getVM(this); }
	
		// TODO: 'engine.version < 3' check for nointerrupt param is a guess and untested - fuzzie
		if (vm->fireScript(s, (event == 9 || engine.version < 3), from)) {
			lastScript = event;
			zotstack();
			vm->setVariables(one, two);

			// TODO: we should set _it_ in a more sensible way
			CreatureAgent *a = dynamic_cast<CreatureAgent *>(this);
			if (a) {
				Creature *c = a->getCreature();
				assert(c);
				vm->_it_ = c->getAttentionFocus();
			}
			
			vmTick();
			ranscript = true;
		} else if (madevm) {
			world.freeVM(vm);
			vm = 0;
		}	
	}

	switch (event) {
		case 5:
			from->dropCarried(this); // TODO: correct?
			break;
	}
	
	return ranscript;
}

bool Agent::queueScript(unsigned short event, AgentRef from, caosVar p0, caosVar p1) {
	// Queue a script for execution on the VM of this agent.

	if (dying) return false;

	// only bother firing the event if either it exists, or it's one with engine code attached
	// TODO: why don't we do the engine checks/etc here?
	switch (event) {
		default:
			if (!findScript(event)) return false;

		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 12:
		case 13:
		case 14:
		case 92:
			world.queueScript(event, this, from, p0, p1);
	}
	
	return true;
}

void Agent::handleClick(float clickx, float clicky) {
	// Handle a mouse click.

	// old-style click handling (c1/c2)
	if (engine.version < 3) {
		int action = -1;

		// look up the relevant action for our ACTV state from the clac table
		if ((unsigned int)actv.getInt() < 3)
			action = clac[actv.getInt()];

		if (action != -1) {
			queueScript(calculateScriptId(action), (Agent *)world.hand());
		}

		return;
	}

	// new-style click handling (c2e)
	if (clik != -1) {
		// TODO: handle CLIK
	} else if (clac[0] != -1) {
		queueScript(calculateScriptId(clac[0]), (Agent *)world.hand());
	}
}

void Agent::playAudio(std::string filename, bool controlled, bool loop) {
	assert(!dying);

	sound.reset();
	world.playAudio(filename, this, controlled, loop);
}

bool agentOnCamera(Agent *targ, bool checkall = false); // caosVM_camera.cpp

void Agent::updateAudio(boost::shared_ptr<AudioSource> s) {
	assert(s);

	s->setPos(x + getWidth() / 2, y + getHeight() / 2, zorder);
	// TODO: we could do with a nicer 'inrange' check
	bool inrange = (x + 500 > world.camera.getX()) && (x + getWidth() - 500 < world.camera.getX() + world.camera.getWidth()) &&
		(y + 500 > world.camera.getY()) && (y + getHeight() - 500 < world.camera.getY() + world.camera.getHeight());
	s->setMute(
		world.camera.getMetaRoom() != world.map.metaRoomAt(x, y)
		&&
		!inrange
		);
	// TODO: setVelocity?
}

Point const Agent::boundingBoxPoint(unsigned int n) {
	return boundingBoxPoint(n, Point(x, y), getWidth(), getHeight());
}

Point const Agent::boundingBoxPoint(unsigned int n, Point in, unsigned int w, unsigned int h) {
	Point p;
	
	switch (n) {
		case 0: // left
			p.x = in.x;
			p.y = in.y + (h / 2.0f);
			break;

		case 1: // right
			p.x = in.x + w;
			p.y = in.y + (h / 2.0f);
			break;

		case 2: // top
			p.x = in.x + (w / 2.0f);
			p.y = in.y;
			break;

		case 3: // bottom
			p.x = in.x + (w / 2.0f);
			p.y = in.y + h;
			break;

		default:
			throw creaturesException("Agent::boundingBoxPoint got unknown direction");
	}

	return p;
}

bool Agent::validInRoomSystem() {
	// Return true if this agent is inside the world room system at present, or false if it isn't.

	// TODO: c1
	if (engine.version == 1) return true;

	return validInRoomSystem(Point(x, y), getWidth(), getHeight(), perm);
}

bool const Agent::validInRoomSystem(Point p, unsigned int w, unsigned int h, int testperm) {
	// Return true if this agent is inside the world room system at the specified point, or false if it isn't.

	for (unsigned int i = 0; i < 4; i++) {
		Point src, dest;
		switch (i) {
			case 0: src = boundingBoxPoint(0, p, w, h); dest = boundingBoxPoint(3, p, w, h); break; // left to bottom
			case 1: src = boundingBoxPoint(1, p, w, h); dest = boundingBoxPoint(3, p, w, h); break; // right to bottom
			case 2: src = boundingBoxPoint(2, p, w, h); dest = boundingBoxPoint(0, p, w, h); break; // top to left
			case 3: src = boundingBoxPoint(2, p, w, h); dest = boundingBoxPoint(1, p, w, h); break; // top to right
		}

		if (engine.version == 2) {
			// Creatures 2 physics
			
			int dx = dest.x - src.x;
			int dy = dest.y - src.y;
			
			Point deltapt(0,0);
			double delta = 1000000000;
			bool collided = false;

			// TODO: check suffercollisions?
			
			// TODO: muh, provided direction here is kinda a hack
			findCollisionInDirection((i < 2) ? 3 : 2, src, dx, dy, deltapt, delta, collided, true);
			if (collided) return false;
		} else {
			// Creatures 3 physics

			float srcx = src.x, srcy = src.y;

			shared_ptr<Room> ourRoom = world.map.roomAt(srcx, srcy);
			if (!ourRoom) return false;

			unsigned int dir; Line wall;
			world.map.collideLineWithRoomSystem(src, dest, ourRoom, src, wall, dir, testperm);

			if (src != dest) return false;
		}
	}

	return true;
}

void Agent::physicsTick() {
	if (engine.version == 1) return; // C1 has no physics, and different attributes.

	if (carriedby) return; // We don't move when carried, so what's the point?

	if (engine.version == 2) {
		// Creatures 2 physics is different.
		physicsTickC2();
		return;
	}

	if (!wasmoved) return; // some agents are created outside INST and get autokilled if we try physics on them before they move

	// set destination point based on velocities
	float destx = x + velx.getFloat();
	float desty = y + vely.getFloat();

	if (sufferphysics()) {
		// TODO: falling behaviour needs looking at more closely..
		// .. but it shouldn't be 'false' by default on non-physics agents, so..
		falling = false;
		// increase speed according to accg
		// TODO: should we be changing vely first, instead of after a successful move (below)?
		desty += accg.getFloat();
	}
	
	if (suffercollisions()) {
		float lastdistance = 1000000.0f;
		bool collided = false;
		Line wall; // only valid when collided
		unsigned int collidedirection = 0; // only valid when collided
		Point bestmove;

		// iterate through all four points of the bounding box
		for (unsigned int i = 0; i < 4; i++) {
			// this mess is because we want to start with the bottom point - DOWN (3) - before the others, for efficiency
			Point src = boundingBoxPoint((i == 0) ? 3 : i - 1);

			// store values
			float srcx = src.x, srcy = src.y;
			
			shared_ptr<Room> ourRoom = world.map.roomAt(srcx, srcy);
			if (!ourRoom) {
				ourRoom = world.map.roomAt(srcx, srcy);
			}
			if (!ourRoom) {
				if (!displaycore) { // TODO: ugh, displaycore is a horrible thing to use for this
					// we're out of the room system, physics bug, but let's try MVSFing back in to cover for fuzzie's poor programming skills
					static bool tryingmove; tryingmove = false; // avoid infinite loop
					if (!tryingmove && tryMoveToPlaceAround(x, y)) {
						//std::cout << identify() << " was out of room system due to a physics bug but we hopefully moved it back in.." << std::endl;
						tryingmove = true;
						physicsTick();
						return;
					}
					
					// didn't work!
					unhandledException(boost::str(boost::format("out of room system at (%f, %f)") % srcx % srcy), false);
				}
				displaycore = true;
				return; // out of room system
			}
			
			Point dest(destx + (srcx - x), desty + (srcy - y));
			unsigned int local_collidedirection;
			Line local_wall;
		
			// this changes src to the point at which we end up
			bool local_collided = world.map.collideLineWithRoomSystem(src, dest, ourRoom, src, local_wall, local_collidedirection, perm);

			float dist;
			if (src.x == srcx && src.y == srcy)
				dist = 0.0f;
			else {
				float xdiff = src.x - srcx;
				float ydiff = src.y - srcy;
				dist = xdiff*xdiff + ydiff*ydiff;
			}

			if (dist >= lastdistance) {
				assert(i != 0); // this had better not be our first collision!
				continue; // further away than a previous collision
			}

			lastdistance = dist;
			bestmove.x = x + (src.x - srcx);
			bestmove.y = y + (src.y - srcy);
			collidedirection = local_collidedirection;
			wall = local_wall;
			collided = local_collided;

			if (dist == 0.0f)
				break; // no point checking any more, is there?
		}	

		// *** do actual movement
		if (lastdistance != 0.0f) {	
			moveTo(bestmove.x, bestmove.y);
		
			if (collided) {
				lastcollidedirection = collidedirection;
				queueScript(6, 0, velx, vely); // TODO: include this? .. we need to include SOMETHING, c3 ball checks for <3

				if (elas != 0) {
					if (wall.getType() == HORIZONTAL) {
						vely.setFloat(-vely.getFloat());
					} else if (wall.getType() == VERTICAL) {
						velx.setFloat(-velx.getFloat());
					} else {
						// line starts always have a lower x value than the end
						float xdiff = wall.getEnd().x - wall.getStart().x;
						float ydiff = wall.getEnd().y - wall.getStart().y;
						float fvelx = velx.getFloat(), fvely = vely.getFloat();
					
						// calculate input/slope angles
						double inputangle;
						if (fvelx == 0.0f) {
							if (fvely > 0.0f)
								inputangle = M_PI / 2.0;
							else
								inputangle = 3 * (M_PI / 2.0);
						} else {
							inputangle = atan(fvely / fvelx);
						}
						double slopeangle = atan(-ydiff / xdiff); // xdiff != 0 because wall isn't vertical

						// calculate output angle
						double outputangle = slopeangle + (slopeangle - inputangle) + M_PI;

						// turn back into component velocities
						double vectorlength = sqrt(fvelx*fvelx + fvely*fvely);
						float xoutput = cos(outputangle) * vectorlength;
						float youtput = sin(outputangle) * vectorlength;

						velx.setFloat(xoutput);
						vely.setFloat(-youtput);
					}

					if (elas != 100.0f) {
						velx.setFloat(velx.getFloat() * (elas / 100.0f));
						vely.setFloat(vely.getFloat() * (elas / 100.0f));
					}
				} else				
					vely.setFloat(0);
			} else if (sufferphysics() && accg != 0) {
				falling = true; // TODO: icky
				vely.setFloat(vely.getFloat() + accg.getFloat());
			}
		} else { velx.setFloat(0); vely.setFloat(0); } // TODO: correct?
	} else {
		if (vely.hasDecimal() || velx.hasDecimal())
			moveTo(destx, desty);
		if (sufferphysics())
			vely.setFloat(vely.getFloat() + accg.getFloat());
	}

	if (sufferphysics() && (aero != 0)) {
		// reduce speed according to AERO
		// TODO: aero should be an integer!
		velx.setFloat(velx.getFloat() - (velx.getFloat() * (aero.getFloat() / 100.0f)));
		vely.setFloat(vely.getFloat() - (vely.getFloat() * (aero.getFloat() / 100.0f)));
	}
}

shared_ptr<Room> const Agent::bestRoomAt(unsigned int tryx, unsigned int tryy, unsigned int direction, shared_ptr<Room> exclude) {
	std::vector<shared_ptr<Room> > rooms = world.map.roomsAt(tryx, tryy);

	shared_ptr<Room> r;

	if (rooms.size() == 0) return shared_ptr<Room>();
	if (rooms.size() == 1) r = rooms[0];
	else if (rooms.size() > 1) {
		unsigned int j;
		for (j = 0; j < rooms.size(); j++) {
			if (rooms[j] == exclude) continue;

			if (direction == 0) { // left
				if (rooms[j]->x_left == tryx) break;
			} else if (direction == 1) { // right
				if (rooms[j]->x_right == tryx) break;
			} else if (direction == 2) { // top
				if (rooms[j]->y_left_ceiling == tryy) break;
			} else { // down
				if (rooms[j]->y_left_floor == tryy) break;
			}
		}
		if (j == rooms.size()) j = 0;
		r = rooms[j];
	}

	if (r == exclude) return shared_ptr<Room>();
	else return r;
}

// Creatures 2 collision finding
void const Agent::findCollisionInDirection(unsigned int i, Point src, int &dx, int &dy, Point &deltapt, double &delta, bool &collided, bool followrooms) {
	src.x = (int)src.x;
	src.y = (int)src.y;
	
	shared_ptr<Room> room = bestRoomAt(src.x, src.y, i, shared_ptr<Room>());

	if (!room) { // out of room system
		if (!displaycore)
			unhandledException(boost::str(boost::format("out of room system at (%f, %f)") % src.x % src.y), false);
		grav.setInt(0);
		displaycore = true;
		return;
	}

	int lastdirection;

	bool steep = abs(dy) > abs(dx);

	int signdx = dx < 0 ? -1 : 1;
	int signdy = dy < 0 ? -1 : 1;

	Line l(Point(0,0),Point(dx,dy));

	Point lastpoint(0,0);

	for (int loc = 0; loc <= abs(steep ? dy : dx); loc++) {
		Point p = steep ? l.pointAtY(loc*signdy) : l.pointAtX(loc*signdx);
		p.x = (int)p.x;
		p.y = (int)p.y;

		bool trycollisions = false;

		bool endofroom = false;
	
		if (src.x + p.x < room->x_left) {
			if (i != 1 && dx < 0) { trycollisions = true; lastdirection = 0; }
			endofroom = true;
		}
		if (src.x + p.x > room->x_right) {
			if (i != 0 && dx > 0) { trycollisions = true; lastdirection = 1; }
			endofroom = true;
		}
		if (src.y + p.y < room->y_left_ceiling) {
			if (i != 3 && dy < 0) { trycollisions = true; lastdirection = 2; }
			endofroom = true;
		}
		if (src.y + p.y > room->y_left_floor) {
			if (i != 2 && dy > 0) { trycollisions = true; lastdirection = 3; }
			endofroom = true;
		}

		// find the next room, if necessary, and work out whether we should move into it or break out
		if (endofroom) {
			shared_ptr<Room> newroom = bestRoomAt(src.x + p.x, src.y + p.y, i, room);

			bool collision = false;

			// collide if we're out of the room system
			if (!newroom) {	collision = true; }
			// collide if there's no new room connected to this one
			else if (room->doors.find(newroom) == room->doors.end()) { collision = true; }
			// collide if the PERM between this room and the new room is smaller than or equal to our size
			else if (size.getInt() > room->doors[newroom]->perm) { collision = true; }

			if (collision && (trycollisions || (!newroom))) {
				collided = true;
				break;
			}

			// move to the new room and keep going!
			room = newroom;
		}
	
		if (room->floorpoints.size() && i == 3 && dy >= 0 && size.getInt() > room->floorvalue.getInt()) { // TODO: Hack!
			// TODO: we don't check floorYatX isn't returning a 'real' room floor, but floorpoints should cover the whole floor anyway
			int floory = room->floorYatX(src.x + p.x);
			
			// never collide when the top point of an object is below the floor.
			Point top = boundingBoxPoint(2);

			// TODO: consider steep floors
			if (src.y + p.y > floory && top.y < floory) {
				collided = true;
				lastdirection = 3;
				break;
			}
		}
		
		if ((!followrooms) && endofroom) break;

		lastpoint = p;
	}

	double length2 = (lastpoint.x * lastpoint.x) + (lastpoint.y * lastpoint.y);
	if (length2 < delta) {
		// TODO: !followrooms is a horrible way to detect a non-physics call
		if (collided && followrooms) lastcollidedirection = lastdirection;
		deltapt = lastpoint;
		delta = length2;
	}
}

void Agent::physicsTickC2() {
	int dx = velx.getInt(), dy = vely.getInt();

	if (dx != 0 || dy != 0) grav.setInt(1);

	if (grav.getInt() != 0 && sufferphysics()) {
		dy += accg.getInt();
	}

	grav.setInt(1);

	if (dy == 0 && dx == 0) { // nothing to do
		if (vely.getInt() == 0) {
			// really no motion
			grav.setInt(0);
		} else {
			// y motion cancelled by gravity
			vely.setInt(dy);
		}
		return;
	}
	vely.setInt(dy);

	Point deltapt(0,0);
	double delta = 1000000000;

	bool collided = false;

	if (suffercollisions()) {
		for (unsigned int i = 0; i < 4; i++) {
			Point src = boundingBoxPoint(i);
			findCollisionInDirection(i, src, dx, dy, deltapt, delta, collided, true);
		}
	} else {
		deltapt.x = dx;
		deltapt.y = dy;
	}
	
	if (collided && (velx.getInt() != 0 || vely.getInt() != 0)) {
		if (lastcollidedirection >= 2) // up and down
			vely.setInt(-(vely.getInt() - (rest.getInt() * vely.getInt()) / 100));
		else
			velx.setInt(-(velx.getInt() - (rest.getInt() * velx.getInt()) / 100));
		queueScript(6, 0);	
	}
	if ((int)deltapt.x == 0 && (int)deltapt.y == 0) {
		grav.setInt(0);
		velx.setInt(0);
		vely.setInt(0);
	} else {
		moveTo(x + (int)deltapt.x, y + (int)deltapt.y);
		if (sufferphysics()) {
			int fricx = (aero.getInt() * velx.getInt()) / 100;
			int fricy = (aero.getInt() * vely.getInt()) / 100;
			if (abs(velx.getInt()) > 0 && fricx == 0) fricx = (velx.getInt() < 0) ? -1 : 1;
			if (abs(vely.getInt()) > 0 && fricy == 0) fricy = (vely.getInt() < 0) ? -1 : 1;
			velx.setInt(velx.getInt() - fricx);
			vely.setInt(vely.getInt() - fricy);
		}
	}
}

void Agent::tick() {
	// sanity checks to stop ticks on dead agents
	LifeAssert la(this);
	if (dying) return;
	
	if (sound) {
		// if the sound is no longer playing...
		if (sound->getState() != SS_PLAY) {
			// ...release our reference to it
			sound.reset();
		} else {
			// otherwise, reposition audio
			updateAudio(sound);
		}
	}

	// don't tick paused agents
	if (paused) return;

	// CA updates
	if (emitca_index != -1 && emitca_amount != 0.0f) {
		assert(0 <= emitca_index && emitca_index <= 19);
		shared_ptr<Room> r = world.map.roomAt(x, y);
		if (r) {
			r->catemp[emitca_index] += emitca_amount;
			/*if (r->catemp[emitca_index] <= 0.0f) r->catemp[emitca_index] = 0.0f;
			else if (r->catemp[emitca_index] >= 1.0f) r->catemp[emitca_index] = 1.0f;*/
		}
	}

	// tick the physics engine
	physicsTick();
	if (dying) return; // in case we were autokilled

	// update the timer if needed, and then queue a timer event if necessary
	if (timerrate) {
		tickssincelasttimer++;
		if (tickssincelasttimer == timerrate) {
			queueScript(9); // TODO: include this?
			tickssincelasttimer = 0;
		}
		assert(timerrate > tickssincelasttimer);
	}

	// tick the agent VM
	if (vm) vmTick();
}

void Agent::unhandledException(std::string info, bool wasscript) {
	// TODO: do something with this? empty the stack?
	if (world.autostop) {
		// autostop is mostly for Creatures Village, which is full of buggy scripts
		stopScript();
	} else if (world.autokill && !dynamic_cast<CreatureAgent *>(this)) { // don't autokill creatures! TODO: someday we probably should :)
		kill();
		if (wasscript)
			std::cerr << identify() << " was autokilled during script " << lastScript << " due to: " << info << std::endl;
		else
			std::cerr << identify() << " was autokilled due to: " << info << std::endl;
	} else {
		stopScript();
		if (wasscript)
			std::cerr << identify() << " caused an exception during script " << lastScript << ": " << info << std::endl;
		else
			std::cerr << identify() << " caused an exception: " << info << std::endl;
	}
}

void Agent::vmTick() {
	// Tick the VM associated with this agent.
	
	assert(vm); // There must *be* a VM to tick.
	LifeAssert la(this); // sanity check

	// If we're out of timeslice, give ourselves some more (depending on the game type).
	if (!vm->timeslice) {
		vm->timeslice = (engine.version < 3) ? 1 : 5;
	}

	// Keep trying to run VMs while we don't run out of timeslice, end up with a blocked VM, or a VM stops.
	while (vm && vm->timeslice && !vm->isBlocking() && !vm->stopped()) {
		assert(vm->timeslice > 0);

		// Tell the VM to tick (using all available timeslice), catching exceptions as necessary.
		try {
			vm->tick();
		} catch (invalidAgentException &e) {
			// try letting the exception script handle it
			if (!queueScript(255))
				unhandledException(e.prettyPrint(), true);
			else
				stopScript(); // we still want current script to die
		} catch (creaturesException &e) {
			unhandledException(e.prettyPrint(), true);
		} catch (std::exception &e) {
			unhandledException(e.what(), true);
		}
		
		// If the VM stopped, it's done.
		if (vm && vm->stopped()) {
			world.freeVM(vm);
			vm = NULL;
		}
	}

	// Zot any remaining timeslice, since we're done now.
	if (vm) vm->timeslice = 0;
	
	// If there's no current VM but there's one on the call stack, a previous VM must have finished,
	// pop one from the call stack to run next time.
	if (!vm && !vmstack.empty()) {
		vm = vmstack.front();
		vmstack.pop_front();
	}
}

Agent::~Agent() {
	assert(lifecount == 0);

	if (!initialized) return;
	if (!dying) {
		// we can't do kill() here because we can't do anything which might try using our shared_ptr
		// (since this could be during world destruction)

		if (vm) {
			vm->stop();
			world.freeVM(vm);
			vm = 0;
		}

		zotstack();

		if (sound) {
			sound->stop();
			sound.reset();
		}
	}
}

void Agent::kill() {
	assert(!dying);
	if (floatable()) floatRelease();
	if (carrying) dropCarried(carrying);
	// TODO: should the carried agent really be responsible for dropping from vehicle?
	if (invehicle) invehicle->drop(this);
	
	dying = true; // what a world, what a world...

	if (vm) {
		vm->stop();
		world.freeVM(vm);
		vm = 0;
	}
	
	zotstack();
	agents_iter->reset();

	if (sound) {
		sound->stop();
		sound.reset();
	}
}

unsigned int Agent::getZOrder() const {
	if (invehicle) {
		// TODO: take notice of cabp in c2e, at least. also, stacking .. ?
		Vehicle *v = dynamic_cast<Vehicle *>(invehicle.get());
		assert(v);
		if (engine.version < 3)
			return v->cabinplane; // TODO: correct?
		else
			return v->getZOrder() + v->cabinplane;
		// TODO: Vehicle should probably rearrange zorder of passengers if ever moved
	} else if (carriedby) {
		// TODO: check for overflow
		// TODO: is adding our own zorder here correct behaviour? someone should check
		if (engine.version > 1)
			return carriedby->getZOrder() - 100;
		else
			return carriedby->getZOrder();
	} else {
		return zorder;
	}
}

void Agent::setZOrder(unsigned int z) {
	if (dying) return;
	zorder = z;
}

int Agent::getUNID() const {
	if (unid != -1)
		return unid;
	return unid = world.newUNID(const_cast<Agent *>(this));
}

#include "Catalogue.h"

std::string Agent::identify() const {
	std::ostringstream o;
	o << (int)family << " " << (int)genus << " " << species;
	const std::string n = catalogue.getAgentName(family, genus, species);
	if (n.size())
		o << " (" + n + ")";
	/*if (unid != -1)
		o << " unid " << unid;
	else
		o << " (no unid assigned)"; */
	return o.str();
}

bool agentzorder::operator ()(const Agent *s1, const Agent *s2) const {
	return s1->getZOrder() < s2->getZOrder();
}

void Agent::pushVM(caosVM *newvm) {
	assert(newvm);
	if (vm)
		vmstack.push_front(vm);
	vm = newvm;
}

bool Agent::vmStopped() {
	return (!vm || vm->stopped());
}

void Agent::stopScript() {
	zotstack();
	if (vm)
		vm->stop();
}

void Agent::addCarried(AgentRef a) {
	assert(a);

	// TODO: muh, vehicle drop needs more thought
	if (a->invehicle) {
		Vehicle *v = dynamic_cast<Vehicle *>(a->invehicle.get());
		assert(v);
		v->dropCarried(a);
	}

	carry(a);

	// TODO: this doesn't reorder children or anything..
	a->setZOrder(a->zorder);

	// fire 'Got Carried Agent'
	if (engine.version >= 3)
		queueScript(124, a); // TODO: is this the correct param?
}

void Agent::carry(AgentRef a) {
	assert(a);
	
	// TODO: check for infinite loops (eg, us carrying an agent which is carrying us) :)

	if (carrying)
		dropCarried(carrying);

	carrying = a;

	a->carriedby = AgentRef(this);
	// TODO: move carrying agent to the right position
	adjustCarried(0, 0);
}

bool Agent::beDropped() {	
	carriedby = AgentRef(0);
	bool wasinvehicle = invehicle;
	invehicle = AgentRef(0);

	// TODO: this doesn't reorder children or anything..
	setZOrder(zorder);

	// TODO: no idea if this is right, it tries to re-enable gravity when dropping C2 agents
	if (engine.version == 2) grav.setInt(1);
	if (engine.version == 3) falling = true;

	if (!wasinvehicle) { // ie, we're not being dropped by a vehicle
		// TODO: check for vehicles in a saner manner?
		for (std::list<boost::shared_ptr<Agent> >::iterator i = world.agents.begin(); i != world.agents.end(); i++) {
			boost::shared_ptr<Agent> a = (*i);
			if (!a) continue;
			Vehicle *v = dynamic_cast<Vehicle *>(a.get());
			if (!v) continue;

			if (agentsTouching(this, v)) {
				v->addCarried(this);
				// TODO: how to handle not-invehicle case, where vehicle has failed to pick us up?
				if (invehicle) return true;
			}
		}
	}
	
	if (engine.version == 1) {
		MetaRoom* m = world.map.metaRoomAt(x, y);
		if (!m) return false;
		shared_ptr<Room> r = m->nextFloorFromPoint(x, y);
		if (!r) return false;
		moveTo(x, r->bot.pointAtX(x).y - getHeight());
	} else {
		// TODO: maybe think about this some more
		tryMoveToPlaceAround(x, y);
	}

	// TODO: return value is not used anywhere yet?
	return true;
}

void Agent::dropCarried(AgentRef a) {
	drop(a);

	// TODO: this doesn't reorder children or anything..
	a->setZOrder(a->zorder);

	// fire 'Lost Carried Agent'
	if (engine.version >= 3)
		queueScript(125, a); // TODO: is this the correct param?
}

void Agent::drop(AgentRef a) {
	if (!carrying) return;
	assert(carrying == a);

	a->beDropped();
	carrying = AgentRef(0);
}

std::pair<int, int> Agent::getCarryPoint() {
	unsigned int ourpose = 0;

	SpritePart *s;
	if ((s = dynamic_cast<SpritePart *>(part(0))))
		ourpose = s->getBase() + s->getPose();

	std::pair<int, int> pos(0, 0);
	
	std::map<unsigned int, std::pair<int, int> >::iterator i = carry_points.find(ourpose);
	if (i != carry_points.end())
		pos = i->second;

	return pos;
}

std::pair<int, int> Agent::getCarriedPoint() {
	unsigned int theirpose = 0;

	SpritePart *s;
	if ((s = dynamic_cast<SpritePart *>(part(0))))
		theirpose = s->getBase() + s->getPose();

	std::pair<int, int> pos(0, 0);

	std::map<unsigned int, std::pair<int, int> >::iterator i = carried_points.find(theirpose);
	if (i != carried_points.end()) {
		pos = i->second;
	} else if (s && engine.version > 1) {
		// TODO: why fudge pickup location here and not in default carried points or something?
		// (this is nornagon's code which i moved - fuzzie)
		pos.first = s->getSprite()->width(s->getCurrentSprite()) / 2;
	}

	return pos;
}

void Agent::adjustCarried(float unusedxoffset, float unusedyoffset) {
	// Adjust the position of the agent we're carrying.
	// TODO: this doesn't actually position the carried agent correctly, sigh

	if (!carrying) return;

	unsigned int ourpose = 0;

	int xoffset = 0, yoffset = 0;
	if (engine.version < 3 && world.hand() == this) {
		// this appears to produce correct behaviour in the respective games, don't ask me  -nornagon
		if (engine.version == 2) {
			xoffset = world.hand()->getWidth() / 2;
			yoffset = world.hand()->getHeight() / 2 - 2;
		} else
			yoffset = world.hand()->getHeight() / 2 - 3;
	}

	std::pair<int, int> carrypoint = getCarryPoint();
	xoffset += carrypoint.first;
	yoffset += carrypoint.second;

	std::pair<int, int> carriedpoint = carrying->getCarriedPoint();
	xoffset -= carriedpoint.first;
	yoffset -= carriedpoint.second;

	if (xoffset != 0 || yoffset != 0)
		carrying->moveTo(x + xoffset, y + yoffset, true);
}

void Agent::setClassifier(unsigned char f, unsigned char g, unsigned short s) {
	family = f;
	genus = g;
	species = s;

	if (engine.version < 3) {
		// TODO: categories for other game versions
		category = -1;
	} else {
		category = world.findCategory(family, genus, species);
	}
}

bool Agent::tryMoveToPlaceAround(float x, float y) {
	if (!suffercollisions()) {
		moveTo(x, y);
		return true;
	}

	// second hacky attempt, move from side to side (+/- width) and up (- height) a little
	unsigned int trywidth = getWidth() * 2; if (trywidth < 100) trywidth = 100;
	unsigned int tryheight = getHeight() * 2; if (tryheight < 100) tryheight = 100;
	for (unsigned int xadjust = 0; xadjust < trywidth; xadjust++) {
		for (unsigned int yadjust = 0; yadjust < tryheight; yadjust++) {
			if (validInRoomSystem(Point(x - xadjust, y - yadjust), getWidth(), getHeight(), perm))
				moveTo(x - xadjust, y - yadjust);
			else if ((xadjust != 0) && validInRoomSystem(Point(x + xadjust, y - yadjust), getWidth(), getHeight(), perm))
				moveTo(x + xadjust, y - yadjust);
			else
				continue;
			return true;
		}
	}

	return false;
}

/* vim: set noet: */
