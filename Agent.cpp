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
#include <sstream>
#include "caosVM.h"
#include "Backend.h"
#include <boost/format.hpp>
#include "Room.h"
#include <float.h>
#include <boost/tuple/tuple.hpp>
using boost::get;

void Agent::core_init() {
	initialized = false;
	lifecount = 0;
}

Agent::Agent(unsigned char f, unsigned char g, unsigned short s, unsigned int p) :
  vm(0), zorder(p), timerrate(0), visible(true), family(f), genus(g), species(s)
{
	core_init();

	lastScript = -1;
	initialized = true;
	velx.setFloat(0.0f);
	vely.setFloat(0.0f);
	accg = 0.3f;
	aero = 0;
	elas = 0;
	perm = 50; // TODO: correct default?
	range = 500;
	falling = true;
	x = 0.0f; y = 0.0f;

	// TODO: is this the correct default?
	clac[0] = 0; // message# for activate 1
	if (engine.version < 3) {
		// TODO: is this the correct default? (this is equivalent to bhvr click == 0)
		clac[0] = -1; clac[1] = -1; clac[2] = -1;
	}
	clik = -1;
	
	dying = false;
	unid = -1;

	soundslot = 0;
	paused = displaycore = false;

	cr_can_push = cr_can_pull = cr_can_stop = cr_can_hit = cr_can_eat = cr_can_pickup = false; // TODO: check this
	imsk_key_down = imsk_key_up = imsk_mouse_move = imsk_mouse_down = imsk_mouse_up = imsk_mouse_wheel = imsk_translated_char = false;

	emitca_index = -1; emitca_amount = 0.0f;

	objp.setAgent(0); // not strictly necessary
}

void Agent::finishInit() {
	// shared_from_this() can only be used if these is at least one extant
	// shared_ptr which owns this
	world.agents.push_front(boost::shared_ptr<Agent>(this));
	agents_iter = world.agents.begin();

	if (findScript(10))
		queueScript(10); // constructor
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

	// if we're being carried and aren't being forced to move (prbly by our carrier), forget it
	if (carriedby && !force) return;
	
	// TODO: what if we move while being carried? doomy explosions ensue, that's what!
	float xoffset = _x - x;
	float yoffset = _y - y;		

	x = _x; y = _y;

	for (std::vector<AgentRef>::iterator i = floated.begin(); i != floated.end(); i++) {
		assert(*i);
		(*i)->moveTo((*i)->x + xoffset, (*i)->y + yoffset);
	}

	adjustCarried();
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
bool Agent::fireScript(unsigned short event, Agent *from) {
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
				if (!carryable()) return false;
			}
			from->carry(this); // TODO: correct behaviour?
			break;
		case 5: // drop
			if (!from) return false;
			if (from != carriedby) return false;
			from->dropCarried(); // TODO: correct?
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

	shared_ptr<script> s = findScript(event);
	if (!s) return false;
	
	bool madevm = false;
	if (!vm) { madevm = true; vm = world.getVM(this); }
	
	if (vm->fireScript(s, (event == 9), from)) {
		lastScript = event;
		zotstack();
		return true;
	} else if (madevm) {
		world.freeVM(vm);
		vm = 0;
	}

	return false;
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

void Agent::positionAudio(SoundSlot *slot) {
	assert(slot);

	// TODO: No idea if this is at all similar to what c2e does, but it seems to sort of work okay.
	float xoff = x + (getWidth() / 2) - world.camera.getXCentre(),
				yoff = y + (getHeight() / 2) - world.camera.getYCentre();
	double dist = sqrt(xoff*xoff + yoff*yoff) / 128;
	int panconst = xoff/10;
	if (panconst > 127) panconst = 127;
	if (panconst < -127) panconst = -127;

	// panning proportional to xoff
	// overall volume proportional to dist
	slot->adjustPanning((127-panconst)/dist, (127+panconst)/dist);

	/*double angle;
	// TODO: this is horribly, horribly broken
	if (xoff == 0) {
		if (yoff > 0) angle = 180;
		else angle = 0;
	} else {
		angle = (atanf(yoff / xoff) * (180 / M_PI));
		if (xoff > 0) // 1st & 2nd quadrants
			angle += 90;
		else // 3rd & 4th quadrants
			angle += 270;
	}

	dist *= (double)(255 / 1000);
	if (dist > 255) dist = 255;*/
}

Point Agent::boundingBoxPoint(unsigned int n) {
	return boundingBoxPoint(n, Point(x, y), getWidth(), getHeight());
}

Point Agent::boundingBoxPoint(unsigned int n, Point in, unsigned int w, unsigned int h) {
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

	return validInRoomSystem(Point(x, y), getWidth(), getHeight(), perm);
}

bool Agent::validInRoomSystem(Point p, unsigned int w, unsigned int h, int testperm) {
	// Return true if this agent is inside the world room system at the specified point, or false if it isn't.

	for (unsigned int i = 0; i < 4; i++) {
		Point src, dest;
		switch (i) {
			case 0: src = boundingBoxPoint(3, p, w, h); dest = boundingBoxPoint(0, p, w, h); break; // bottom to left
			case 1: src = boundingBoxPoint(3, p, w, h); dest = boundingBoxPoint(1, p, w, h); break; // bottom to right
			case 2: src = boundingBoxPoint(2, p, w, h); dest = boundingBoxPoint(0, p, w, h); break; // top to left
			case 3: src = boundingBoxPoint(2, p, w, h); dest = boundingBoxPoint(1, p, w, h); break; // top to right
		}
		float srcx = src.x, srcy = src.y;
		
		shared_ptr<Room> ourRoom = world.map.roomAt(srcx, srcy);
		if (!ourRoom) return false;

		unsigned int dir; Line wall;
		world.map.collideLineWithRoomSystem(src, dest, ourRoom, src, wall, dir, testperm);

		if (src != dest) return false;
	}

	return true;
}

// dist2: distance²
static inline float dist2(float x1, float y1, float x2, float y2) {
	float dx = x2-x1, dy = y2-y1;
	return dx*dx + dy*dy;
}
static inline float dist2(Point const &a, Point const &b) {
	return dist2(a.x, a.y, b.x, b.y);
}

void Agent::physicsTick() {
	if (engine.version == 1) return; // C1 has no physics, and different attributes.
	if (carriedby) return; // We don't move when carried, so what's the point?
	if (x == 0 && y == 0) return; // TODO: is this correct behaviour? :P

	// set destination point based on velocities
	float dx = velx.getFloat();
	float dy = vely.getFloat();

	if (sufferphysics()) {
		// TODO: falling behaviour needs looking at more closely..
		// .. but it shouldn't be 'false' by default on non-physics agents, so..
		falling = false;
		// increase speed according to accg
		// TODO: should we be changing vely first, instead of after a successful move (below)?
		dy += accg.getFloat();
	}
	
	if (suffercollisions()) {
		float lastdistance = INFINITY;
		Point newpoint(x + dx, y + dy);
		int collidedirection;
		int pointcollided = 5;
		Line* wall = NULL; // only valid when collided
		Vector<float> velocityNorm = Vector<float>(dx,dy).scaleToMagnitude(1);

		std::vector<boost::tuple<int, Line*, shared_ptr<Room> > > lines;
		MetaRoom *mr = world.map.metaRoomAt(x, y);
		assert(mr);
		// snaffle all the lines in the current metaroom
		for (std::vector<shared_ptr<Room> >::iterator r = mr->rooms.begin(); r != mr->rooms.end(); r++) {
			lines.push_back(make_tuple(0, &(*r)->left, *r));
			lines.push_back(make_tuple(1, &(*r)->right, *r));
			lines.push_back(make_tuple(2, &(*r)->top, *r));
			lines.push_back(make_tuple(3, &(*r)->bot, *r));
		}

		std::vector<Line> outline;
		outline.push_back(Line(boundingBoxPoint(0), boundingBoxPoint(2)));
		outline.push_back(Line(boundingBoxPoint(2), boundingBoxPoint(1)));
		outline.push_back(Line(boundingBoxPoint(1), boundingBoxPoint(3)));
		outline.push_back(Line(boundingBoxPoint(3), boundingBoxPoint(0)));

		for (std::vector<boost::tuple<int, Line*, shared_ptr<Room> > >::iterator rl = lines.begin(); rl != lines.end(); rl++) {
			for (int j = 3; j >= 0; j--) {
				Point src = boundingBoxPoint(j);
				Line vel(src, Point(src.x + dx, src.y + dy));
				Point i;
				if (vel.intersect(*get<1>(*rl), i)) {
					float ourdist = dist2(src, i);
					if (ourdist < lastdistance) {
						shared_ptr<Room> from_room = get<2>(*rl);//world.map.roomAt(i.x - velocityNorm.x, i.y - velocityNorm.y);
						if (!from_room) {
							printf("i(%f,%f) ; wall from (%f,%f) to (%f,%f) ; dx = %f, dy = %f\n", i.x, i.y, get<1>(*rl)->getStart().x, get<1>(*rl)->getStart().y, get<1>(*rl)->getEnd().x, get<1>(*rl)->getEnd().y, dx, dy);
						}
						assert(from_room);
						bool wall_permeable = false;
						switch (get<0>(*rl)) {
							case 0: // collided with the left wall of a room
								for (std::map<boost::weak_ptr<Room>,RoomDoor *>::iterator rd = from_room->doors.begin();
										rd != from_room->doors.end(); rd++) {
									if (!(rd->first.lock()) || !(rd->second)) continue; // TODO: assert here? remove the door?
									Line *l = &(rd->first.lock()->right);
									if (l->getStart().x == i.x && l->containsY(i.y) &&
											rd->second->perm > perm) {
										wall_permeable = true;
										break;
										// move along, no collision here
									}
								}
								break;
							case 1: // right wall
								// TODO: copypasta
								for (std::map<boost::weak_ptr<Room>,RoomDoor *>::iterator rd = from_room->doors.begin();
										rd != from_room->doors.end(); rd++) {
									if (!rd->first.lock() || !rd->second) continue; // TODO: assert here? remove the door?
									Line *l = &(rd->first.lock()->left);
									if (l->getStart().x == i.x && l->containsY(i.y) &&
											rd->second->perm > perm) {
										wall_permeable = true;
										break;
										// move along, no collision here
									}
								}
								break;
							case 2: // top wall
								{
									shared_ptr<Room> to_room = world.map.roomAt(i.x, i.y - 1); // all rooms are at least three pixels tall everywhere
									for (std::map<boost::weak_ptr<Room>,RoomDoor *>::iterator rd = from_room->doors.begin();
											rd != from_room->doors.end(); rd++) {
										if (!rd->first.lock() || !rd->second) continue;
										if (rd->first.lock() == to_room && rd->second->perm > perm) {
											wall_permeable = true;
											break;
										}
									}
								}
								break;
							case 3: // bottom wall
								// TODO: copypasta
								{
									shared_ptr<Room> to_room = world.map.roomAt(i.x, i.y + 1); // all rooms are at least three pixels tall everywhere
									if (!to_room) break;// printf("no to_room\n");
									for (std::map<boost::weak_ptr<Room>,RoomDoor *>::iterator rd = from_room->doors.begin();
											rd != from_room->doors.end(); rd++) {
										if (!rd->first.lock() || !rd->second) continue;
										if (rd->first.lock() == to_room && rd->second->perm > perm) {
											wall_permeable = true;
											break;
										}
									}
								}
								break;
						}
						if (wall_permeable) continue;
						collidedirection = get<0>(*rl);
						switch (j) {
							case 0: // left
								newpoint = Point(i.x, i.y - getHeight() / 2);
								break;
							case 1: // right
								newpoint = Point(i.x - getWidth(), i.y - getHeight() / 2);
								break;
							case 2: // top
								newpoint = Point(i.x - getWidth() / 2, i.y);
								break;
							case 3: // bottom
								newpoint = Point(i.x - getWidth() / 2, i.y - getHeight());
								break;
							default:
								assert(0 && "unreachable");
						}
						pointcollided = j;
						wall = get<1>(*rl);
						lastdistance = ourdist;
					}
				}
			}
			for (std::vector<Line>::iterator ol = outline.begin(); ol != outline.end(); ol++) {
				Point const &start = get<1>(*rl)->getStart();
				Line l(start, Point(start.x - dx, start.y - dy));
				Point i;
				if (ol->intersect(l, i)) {
					float ourdist = dist2(i, start);
					collidedirection = get<0>(*rl); // TODO: needs testing in c2e
					if (ourdist < lastdistance) {
						wall = get<1>(*rl);
						lastdistance = ourdist;
						newpoint = Point(start.x - i.x + x , start.y - i.y + y);
					}
				}
			}
		}

		// *** do actual movement
		if (lastdistance == INFINITY) { // no collision
			//printf("No collision, moving to (%f,%f)\n", newpoint.x, newpoint.y);
			moveTo(newpoint.x, newpoint.y);
			if (sufferphysics()) vely.setFloat(vely.getFloat() + accg.getFloat());
			//printf("New velocity (after gravity): %f,%f\n", velx.getFloat(), vely.getFloat());
			falling = true;
		} else if (lastdistance < FLT_EPSILON) { // collision at current location
			assert(wall);
			//printf("Wall: (%f, %f) to (%f, %f)\n", wall->getStart().x, wall->getStart().y, wall->getEnd().x, wall->getEnd().y);
			//printf("lastdistance zero, vel (%f,%f)\n", velx.getFloat(), vely.getFloat());
			//velx.setFloat(0); vely.setFloat(0); // TODO: correct?
		} else if (fabs(velx.getFloat()) > FLT_EPSILON || fabs(vely.getFloat()) > FLT_EPSILON) {
			Vector<float> normal;
			assert(wall);
			if (wall->getType() == HORIZONTAL) {
				if (dy < 0)
					normal.y = 1;
				else
					normal.y = -1;
				normal.x = 0;
			} else if (wall->getType() == VERTICAL) {
				if (dx < 0)
					normal.x = 1;
				else
					normal.x = -1;
				normal.y = 0;
			} else {
				normal = Vector<float>(wall->getEnd().y - wall->getStart().y, -(wall->getEnd().x - wall->getStart().x)).scaleToMagnitude(1);
				if (normal.y * dy + normal.x * dx > (-normal.y) * dy + (-normal.x) * dx) {
					//printf("flipped normal\n");
					normal.x = -normal.x;
					normal.y = -normal.y;
				}
			}
			newpoint.x += normal.x * 0.1;
			newpoint.y += normal.y * 0.1;
			if (family == 2 && genus == 21 && species == 20) {
				printf("Normal vector: %f,%f\n", normal.x, normal.y);
				printf("Collision, moving to (%f,%f)\n", newpoint.x, newpoint.y);
			}
			moveTo(newpoint.x, newpoint.y);
			if (elas == 0) {
				vely.setFloat(0);
				velx.setFloat(0);
			} else {
				if (wall->getType() == HORIZONTAL) {
					velx.setFloat(velx.getFloat() * elas / 100.0);
					vely.setFloat(-vely.getFloat() * elas / 100.0);
				if (family == 2 && genus == 21 && species == 20) {
					switch (pointcollided) {
						case 0:
							printf("Left: (%f,%f)\n", newpoint.x, newpoint.y + getHeight() / 2);
							break;
						case 1:
							printf("Right: (%f,%f)\n", newpoint.x + getWidth(), newpoint.y + getHeight() / 2);
							break;
						case 2:
							printf("Top: (%f,%f)\n", newpoint.x + getWidth() / 2, newpoint.y);
							break;
						case 3:
							printf("Bottom: (%f,%f)\n", newpoint.x + getWidth() / 2, newpoint.y + getHeight());
							break;
						default:;
							printf("Abnormal collision direction: %d\n", pointcollided);
					}
					printf("Wall: (%f, %f) to (%f, %f)\n", wall->getStart().x, wall->getStart().y, wall->getEnd().x, wall->getEnd().y);
					printf("Collided with horizontal wall, new velocity: %f,%f\n", velx.getFloat(), vely.getFloat());
				}
				} else if (wall->getType() == VERTICAL) {
					velx.setFloat(-velx.getFloat() * elas / 100.0);
					vely.setFloat(vely.getFloat() * elas / 100.0);
				if (family == 2 && genus == 21 && species == 20) {
					switch (pointcollided) {
						case 0:
							printf("Left: (%f,%f)\n", newpoint.x, newpoint.y + getHeight() / 2);
							break;
						case 1:
							printf("Right: (%f,%f)\n", newpoint.x + getWidth(), newpoint.y + getHeight() / 2);
							break;
						case 2:
							printf("Top: (%f,%f)\n", newpoint.x + getWidth() / 2, newpoint.y);
							break;
						case 3:
							printf("Bottom: (%f,%f)\n", newpoint.x + getWidth() / 2, newpoint.y + getHeight());
							break;
						default:;
							printf("Abnormal collision direction: %d\n", pointcollided);
					}
					printf("Wall: (%f, %f) to (%f, %f)\n", wall->getStart().x, wall->getStart().y, wall->getEnd().x, wall->getEnd().y);
					printf("Collided with vertical wall, new velocity: %f,%f\n", velx.getFloat(), vely.getFloat());
				}
				} else {
					// I' = I - 2*(N·I)*N
					// I is the incident vector
					// N is the normal vector
					// I' is the reflected vector
					float incident_x = dx;
					float incident_y = dy;
					float two_i_dot_n = 2 * (incident_x * normal.x + incident_y * normal.y);
					float two_ni_n_x = two_i_dot_n * normal.x;
					float two_ni_n_y = two_i_dot_n * normal.y;
					float i_dash_x = incident_x - two_ni_n_x;
					float i_dash_y = incident_y - two_ni_n_y;
					i_dash_x *= (elas / 100.0);
					i_dash_y *= (elas / 100.0);
					if (fabs(i_dash_y) < 1)
						i_dash_y = 0;
					if (fabs(i_dash_x) < 1)
						i_dash_x = 0;
			if (family == 2 && genus == 21 && species == 20) {
					switch (pointcollided) {
						case 0:
							printf("Left: (%f,%f)\n", newpoint.x, newpoint.y + getHeight() / 2);
							break;
						case 1:
							printf("Right: (%f,%f)\n", newpoint.x + getWidth(), newpoint.y + getHeight() / 2);
							break;
						case 2:
							printf("Top: (%f,%f)\n", newpoint.x + getWidth() / 2, newpoint.y);
							break;
						case 3:
							printf("Bottom: (%f,%f)\n", newpoint.x + getWidth() / 2, newpoint.y + getHeight());
							break;
						default:;
							printf("Abnormal collision direction: %d\n", pointcollided);
					}
					printf("Wall: (%f,%f) to (%f,%f)\n", wall->getStart().x, wall->getStart().y, wall->getEnd().x, wall->getEnd().y);
					printf("Collided with a wall, velocity was %f,%f now %f,%f\n", velx.getFloat(), vely.getFloat(), i_dash_x, i_dash_y);
			}
					velx.setFloat(i_dash_x);
					vely.setFloat(i_dash_y);
				}
			}
			if (sufferphysics()) {
				/*vely.setFloat(vely.getFloat() + accg.getFloat());
				printf("Gravity suffered, new velocity: %f,%f\n", velx.getFloat(), vely.getFloat());*/
			}
		}
	} else {
		if (vely.hasDecimal() || velx.hasDecimal()) {
			moveTo(x + dx, y + dy);
			//printf("No collisions suffered, moving to (%f,%f)\n", x + dx, y + dy);
		}
	}
	//fflush(stdout);
	//printf("END\n");

	if (sufferphysics() && (aero != 0)) {
		// reduce speed according to AERO
		// TODO: aero should be an integer!
		velx.setFloat(velx.getFloat() - (velx.getFloat() * (aero.getFloat() / 100.0f)));
		vely.setFloat(vely.getFloat() - (vely.getFloat() * (aero.getFloat() / 100.0f)));
	}
}

void Agent::tick() {
	// sanity checks to stop ticks on dead agents
	LifeAssert la(this);
	if (dying) return;
	
	// reposition audio if we're playing any
	if (soundslot) positionAudio(soundslot);

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
	}

	// tick the agent VM
	if (vm) vmTick();
}

void Agent::unhandledException(std::string info, bool wasscript) {
	// TODO: do something with this? empty the stack?
	if (world.autokill) {
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
				unhandledException(std::string("\n") + e.prettyPrint(), true);
			else
				stopScript(); // we still want current script to die
		} catch (caosException &e) {
			// XXX: prettyPrint() isn't being virtual, wtf?
			unhandledException(std::string("\n") + e.prettyPrint(), true);
		} catch (creaturesException &e) {
			unhandledException(std::string("\n") + e.prettyPrint(), true);
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
	
	if (vm)
		world.freeVM(vm);
	zotstack();
	zotrefs();
}

void Agent::kill() {
	assert(!dying);
	if (floatable()) floatRelease();
	dropCarried();
	
	dying = true; // what a world, what a world...

	if (vm) {
		vm->stop();
		world.freeVM(vm);
		vm = 0;
	}
	
	zotstack();
	zotrefs();
	agents_iter->reset();
	if (soundslot) soundslot->stop();
}

void Agent::zotrefs() {
}

unsigned int Agent::getZOrder() const {
	if (carriedby) {
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
	  return unid = world.getUNID(const_cast<Agent *>(this));
}

std::string Agent::identify() const {
	std::ostringstream o;
	o << (int)family << " " << (int)genus << " " << species;
	const std::string n = world.catalogue.getAgentName(family, genus, species);
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

void Agent::stopScript() {
	zotstack();
	if (vm)
		vm->stop();
}

void Agent::carry(AgentRef a) {
	// TODO: check for infinite loops (eg, us carrying an agent which is carrying us) :)

	if (carrying)
		dropCarried();

	carrying = a;
	if (!carrying) return;

	a->carriedby = AgentRef(this);
	// TODO: move carrying agent to the right position
	// TODO: this doesn't reorder children or anything..
	carrying->setZOrder(carrying->zorder);
	adjustCarried();

	// fire 'Got Carried Agent'
	queueScript(124, carrying); // TODO: is this the correct param?
}

void Agent::dropCarried() {
	if (!carrying) return;
	
	if (engine.version == 1) {
		MetaRoom* m = world.map.metaRoomAt(carrying->x,carrying->y);
		if (!m) return;
		shared_ptr<Room> r = m->nextFloorFromPoint(carrying->x,carrying->y);
		if (!r) return; // TODO: hack to avoid black holes, is this correct?
		carrying->carriedby = AgentRef(0);
		carrying->moveTo(carrying->x,r->bot.pointAtX(carrying->x).y - carrying->getHeight());
	} else
		carrying->carriedby = AgentRef(0);

	// TODO: this doesn't reorder children or anything..
	carrying->setZOrder(carrying->zorder);

	// fire 'Lost Carried Agent'
	queueScript(125, carrying); // TODO: is this the correct param?
	carrying = AgentRef(0);
}

void Agent::adjustCarried() {
	// Adjust the position of the agent we're carrying.
	// TODO: this doesn't actually position the carried agent correctly, sigh

	if (!carrying) return;

	unsigned int ourpose = 0, theirpose = 0;

	SpritePart *s;
	if ((s = dynamic_cast<SpritePart *>(part(0))))
		ourpose = s->getBase() + s->getPose();
	if ((s = dynamic_cast<SpritePart *>(carrying->part(0))))
		theirpose = s->getBase() + s->getPose();

	int xoffset = 0, yoffset = 0;
	if (engine.version < 3 && world.hand() == this) {
		// this appears to produce correct behaviour in the respective games, don't ask me  -nornagon
		if (engine.version == 2) {
			xoffset = world.hand()->getWidth() / 2;
			yoffset = world.hand()->getHeight() / 2 - 2;
		} else
			yoffset = world.hand()->getHeight() / 2 - 3;
	}
	
	std::map<unsigned int, std::pair<int, int> >::iterator i = carry_points.find(ourpose);
	if (i != carry_points.end()) {
		xoffset += i->second.first;
		yoffset += i->second.second;
	}

	i = carrying->carried_points.find(theirpose);
	if (i != carrying->carried_points.end()) {
		xoffset -= i->second.first;
		yoffset -= i->second.second;
	} else if (s) {
		if (engine.version > 1)
			xoffset -= s->getSprite()->width(s->getCurrentSprite()) / 2;
	}

	carrying->moveTo(x + xoffset, y + yoffset, true);
}

/* vim: set noet: */
