/*
 *  caosVM_map.cpp
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

#include "caosVM.h"
#include "World.h"
#include "MetaRoom.h"
#include "Room.h"
#include <assert.h>
#include <iostream>
#include <boost/format.hpp>
using std::cerr;

/**
 ADDM (integer) x (integer) y (integer) width (integer) height (integer) background (string)
 %status maybe
 
 Creates a metaroom with the given height and width, at the coordinates given.  Returns the id of the new metaroom.
 */
void caosVM::v_ADDM() {
	VM_VERIFY_SIZE(5)
	VM_PARAM_STRING(background)
	VM_PARAM_INTEGER(height)
	VM_PARAM_INTEGER(width)
	VM_PARAM_INTEGER(y)
	VM_PARAM_INTEGER(x)

	MetaRoom *r = new MetaRoom(x, y, width, height, background);
	caosVar v;
	v.setInt(world.map.addMetaRoom(r));
	result = v;
}

/**
 ADDB (command) metaroom_id (integer) background (string)
 %status maybe

 Adds a new background to an existing metaroom, to be displayed with BKGD.
*/
void caosVM::c_ADDB() {
	VM_PARAM_STRING(background)
	VM_PARAM_INTEGER(metaroomid)
	
	MetaRoom *m = world.map.getMetaRoom(metaroomid);
	caos_assert(m);

	m->addBackground(background);
}

/**
 BRMI (command) metaroom_base (integer) room_base (integer)
 %status maybe
 
 Sets the base ID numbers for new metarooms and rooms to the given values.
 */
void caosVM::c_BRMI() {
	VM_VERIFY_SIZE(2)
	
	VM_PARAM_INTEGER(room_base)
	VM_PARAM_INTEGER(metaroom_base)

	world.map.room_base = room_base;
	world.map.metaroom_base = metaroom_base;
}

/**
 MAPD (command) width (integer) height (integer)
 %status maybe
 
 Sets the world map dimensions, inside which metarooms are placed.
 */
void caosVM::c_MAPD() {
	VM_VERIFY_SIZE(2)
	VM_PARAM_INTEGER(height)
	VM_PARAM_INTEGER(width)

	world.map.SetMapDimensions(width, height);
}

/**
 MAPW (integer)
 %status maybe
 
 Returns the width of the world map.
*/
void caosVM::v_MAPW() {
	result.setInt(world.map.getWidth());
}

/**
 MAPH (integer)
 %status maybe

 Returns the height of the world map.
*/
void caosVM::v_MAPH() {
	result.setInt(world.map.getHeight());
}

/**
 MAPK (command)  
 %status maybe
 
 Resets and empties the world map.
 */
void caosVM::c_MAPK() {
	VM_VERIFY_SIZE(0)

	world.map.Reset();
}

/**
 BKDS (string) metaroomid (integer)
 %status maybe

 Determines all of the background names in use by the given metaroom, and returns them in a comma-seperated string.
*/
void caosVM::v_BKDS() {
	VM_PARAM_INTEGER(metaroomid)
	
	MetaRoom *m = world.map.getMetaRoom(metaroomid);
	caos_assert(m);

	std::vector<std::string> backs = m->backgroundList();
	std::string s;
	for (std::vector<std::string>::iterator i = backs.begin(); i != backs.end(); i++) {
		if (s.empty())
			s = *i;
		else
			s = s + "," + *i;
	}
			
	result.setString(s);
}

/**
 ADDR (integer) metaroomid (integer) x_left (integer) x_right (integer) y_left_ceiling (integer) y_right_ceiling (integer) y_left_floor (integer) y_right_floor (integer)
 %status maybe

 Makes a new room inside the given metaroom.  Rooms can have sloped floors and ceilings, but only vertical walls.  
 The id of the new room is returned.
*/
void caosVM::v_ADDR() {
	VM_VERIFY_SIZE(7)
	VM_PARAM_INTEGER(y_right_floor)
	VM_PARAM_INTEGER(y_left_floor)
	VM_PARAM_INTEGER(y_right_ceiling)
	VM_PARAM_INTEGER(y_left_ceiling)
	VM_PARAM_INTEGER(x_right)
	VM_PARAM_INTEGER(x_left)
	VM_PARAM_INTEGER(metaroomid)

	shared_ptr<Room> r(new Room(x_left, x_right,
			y_left_ceiling, y_right_ceiling,
			y_left_floor, y_right_floor));
	MetaRoom *m = world.map.getMetaRoom(metaroomid);
	caos_assert(m);
	r->metaroom = m;
	r->id = m->addRoom(r);
	result.setInt(r->id);
}

/**
 RTYP (command) roomid (integer) roomtype (integer)
 %status maybe

 Defines the 'type' of the given room.  The types vary with different games.
*/
void caosVM::c_RTYP() {
	VM_VERIFY_SIZE(2)
	VM_PARAM_INTEGER(roomtype)
	VM_PARAM_INTEGER(roomid)

	shared_ptr<Room> room = world.map.getRoom(roomid);
	caos_assert(room);
	room->type = roomtype;
}

/**
 RTYP (integer) roomid (integer)
 %status maybe

 Returns the 'type' of the given room, or -1 if 'roomid' is invalid.
*/
void caosVM::v_RTYP() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(roomid)

	shared_ptr<Room> room = world.map.getRoom(roomid);
	if (room)
		result.setInt(room->type.getInt());
	else
		result.setInt(-1);
}

/**
 RTYP (integer)
 %status maybe
 %pragma variants c2
 %pragma implementation caosVM::v_RTYP_c2

 Returns the room type of the room at the centre point of targ.
*/
void caosVM::v_RTYP_c2() {
	valid_agent(targ);
	shared_ptr<Room> r = world.map.roomAt(targ->x + (targ->getWidth() / 2.0f), targ->y + (targ->getHeight() / 2.0f));
	if (!r) result.setInt(-1);
	else {
		result.setInt(r->type.getInt());
	}
}

/**
 SETV RTYP (command) roomtype (integer)
 %status maybe
 %pragma variants c2

 Sets the type of the given room to roomtype.
*/
void caosVM::c_SETV_RTYP() {
	VM_VERIFY_SIZE(1);
	VM_PARAM_INTEGER(roomtype);

	// TODO: this does actually work on targ, right?
	// seems to work for the airlock, anyway  -nornagon
	valid_agent(targ);
	shared_ptr<Room> r = world.map.roomAt(targ->x + (targ->getWidth() / 2.0f), targ->y + (targ->getHeight() / 2.0f));
	if (!r) return; // TODO: correct behaviour?
	else
		r->type.setInt(roomtype);
}

/**
 DOOR (command) room1 (integer) room2 (integer) perm (integer)
 %status maybe

 Sets how permeable the door between the two given rooms will be. (See PERM).
*/
void caosVM::c_DOOR() {
	VM_VERIFY_SIZE(3)
	VM_PARAM_INTEGER(perm)
	VM_PARAM_INTEGER(room2)
	VM_PARAM_INTEGER(room1)

	shared_ptr<Room> r1 = world.map.getRoom(room1);
	shared_ptr<Room> r2 = world.map.getRoom(room2);
	caos_assert(r1); caos_assert(r2);
	if (r1->doors.find(r2) == r1->doors.end()) {
		RoomDoor *door = new RoomDoor;
		door->first = r1;
		door->second = r2;
		door->perm = perm;
		r1->doors[r2] = door;
		r2->doors[r1] = door;
	} else {
		RoomDoor *door = r1->doors[r2];
		door->perm = perm;
	}
}

/**
 DOOR (integer) room1 (integer) room2 (integer)
 %status stub
*/
void caosVM::v_DOOR() {
	VM_PARAM_INTEGER(room2)
	VM_PARAM_INTEGER(room1)

	result.setInt(-1); // TODO
}

/**
 RATE (command) roomtype (integer) caindex (integer) gain (float) loss (float) diffusion (float)
 %status maybe

 Defines the rates of the given CA in the given room.  'gain' defines how easily the CA will be absorbed from
 agents inside the room, 'loss' defines how much will be lost into the air, and 'diffusion' defines how easily it 
 will spread to other rooms.
*/
void caosVM::c_RATE() {
	VM_VERIFY_SIZE(5)
	VM_PARAM_FLOAT(diffusion)
	VM_PARAM_FLOAT(loss)
	VM_PARAM_FLOAT(gain)
	VM_PARAM_INTEGER(caindex)
	VM_PARAM_INTEGER(roomtype)

	cainfo info;
	info.gain = gain;
	info.loss = loss;
	info.diffusion = diffusion;
	world.carates[roomtype][caindex] = info;
}

shared_ptr<Room> roomContainingAgent(AgentRef agent) {
	return world.map.roomAt(agent->x + (agent->getWidth() / 2.0f), agent->y + (agent->getHeight() / 2.0f));
}

/**
 ROOM (integer) agent (agent)
 %status maybe
 
 Returns the room that contains the given agent (jugding by its center).
*/
void caosVM::v_ROOM() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_VALIDAGENT(agent)
	
	shared_ptr<Room> r = roomContainingAgent(agent);
	if (r)
		result.setInt(r->id);
	else
		result.setInt(-1);
}

/**
 LEFT (integer)
 %status maybe
 
 Returns the left constant (0).
*/
void caosVM::v_LEFT() {
	VM_VERIFY_SIZE(0)
	
	result.setInt(0);
}

/**
 RGHT (integer)
 %status maybe
 
 Returns the right constant (1).
*/
void caosVM::v_RGHT() {
	VM_VERIFY_SIZE(0)
	
	result.setInt(1);
}

/**
 _UP_ (integer)
 %status maybe
 %pragma implementation caosVM::v_UP
 
 Returns the up constant (2).
*/
void caosVM::v_UP() {
	VM_VERIFY_SIZE(0)
	
	result.setInt(2);
}

/**
 DOWN (integer)
 %status maybe
 
 Returns the down constant (3).
*/
void caosVM::v_DOWN() {
	VM_VERIFY_SIZE(0)
	
	result.setInt(3);
}

/**
 PROP (command) roomid (integer) caindex (integer) cavalue (float)
 %status maybe

 Defines the level of the given CA in the given room.  Valid settings are between 0 and 1; if higher, it will be 
 reset to 1.
*/
void caosVM::c_PROP() {
	VM_VERIFY_SIZE(3)
	VM_PARAM_FLOAT(cavalue)
	VM_PARAM_INTEGER(caindex)
	VM_PARAM_INTEGER(roomid)

	if (cavalue > 1.0f)
		cavalue = 1.0f;
	else
		caos_assert(0.0f <= cavalue);
	caos_assert(0 <= caindex && caindex <= 19);

	shared_ptr<Room> room = world.map.getRoom(roomid);
	caos_assert(room);
	room->ca[caindex] = cavalue;
}

/**
 PROP (float) roomid (integer) caindex (integer)
 %status maybe

 Returns the level of the given CA in the given room, or 0 if a roomid of -1 is passed.
*/
void caosVM::v_PROP() {
	VM_VERIFY_SIZE(2)
	VM_PARAM_INTEGER(caindex)
	VM_PARAM_INTEGER(roomid)
	
	caos_assert(0 <= caindex && caindex <= 19);

	if (roomid == -1) {
		result.setFloat(0.0f);
		return;
	}
	
	shared_ptr<Room> room = world.map.getRoom(roomid);
	caos_assert(room);
		result.setFloat(room->ca[caindex]);
}

/**
 PERM (command) perm (integer)
 %status maybe

 Sets the TARG agent's permiability.  Valid settings are between 1 and 100.
*/
void caosVM::c_PERM() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(perm)

	// TODO: setting of 0 valid?
	if (perm < 0) perm = 0;
	if (perm > 100) perm = 100;

	valid_agent(targ);
	targ->perm = perm;
}

/**
 PERM (integer)
 %status maybe

 Returns the TARG agent's permiability.
*/
void caosVM::v_PERM() {
	VM_VERIFY_SIZE(0)

	valid_agent(targ);
	result.setInt(targ->perm);
}

/**
 GRAP (integer) x (float) y (float)
 %status maybe

 Returns the id of the room at the coordinates (x, y), or -1 if nothing's there.
*/
void caosVM::v_GRAP() {
	VM_VERIFY_SIZE(2)
	VM_PARAM_FLOAT(y)
	VM_PARAM_FLOAT(x)

	shared_ptr<Room> room = world.map.roomAt(x, y);
	if (room) {
		result.setInt(room->id);
	} else {
		result.setInt(-1);
	}
}

/**
 GMAP (integer) x (float) y (float)
 %status maybe

 Returns the id of the metaroom at the coordinates (x, y), or -1 if nothing's there.
*/
void caosVM::v_GMAP() {
	VM_VERIFY_SIZE(2)
	VM_PARAM_FLOAT(y)
	VM_PARAM_FLOAT(x)

	MetaRoom *room = world.map.metaRoomAt(x, y);
	if (room) {
		result.setInt(room->id);
	} else {
		result.setInt(-1);
	}
}

/**
 LINK (command) room1 (integer) room2 (integer) perm (integer)
 %status stub

 Defines the permeability of the link between the two given rooms.  This is used for CA diffusion.
*/
void caosVM::c_LINK() {
	VM_VERIFY_SIZE(3)
	VM_PARAM_INTEGER(perm)
	VM_PARAM_INTEGER(room2)
	VM_PARAM_INTEGER(room1)

	shared_ptr<Room> one = world.map.getRoom(room1);
	shared_ptr<Room> two = world.map.getRoom(room2);
	caos_assert(one && two);

	// TODO
}

/**
 LINK (integer) room1 (integer) room2 (integer)
 %status stub

 Returns the permeability of the link between the given two rooms, or 0 if no link exists.
*/
void caosVM::v_LINK() {
	VM_PARAM_INTEGER(room2)
	VM_PARAM_INTEGER(room1)

	shared_ptr<Room> one = world.map.getRoom(room1);
	shared_ptr<Room> two = world.map.getRoom(room2);
	caos_assert(one && two);

	result.setInt(0); // TODO
}

/**
 GRID (integer) agent (agent) direction (integer)
 %status maybe

 Returns the nearest adjacent room to the specified agent in the given direction (one of the direction constants), or 
 -1 otherwise.
*/
void caosVM::v_GRID() {
	VM_VERIFY_SIZE(2)
	VM_PARAM_INTEGER(direction) caos_assert(direction >= 0); caos_assert(direction <= 3);
	VM_PARAM_VALIDAGENT(agent)

	valid_agent(targ);
	Point src = targ->boundingBoxPoint(direction);
	Point dest = src;
	
	switch (direction) {
		case 0: // left
			dest.x -= targ->range.getFloat(); break;
		case 1: // right
			dest.x += targ->range.getFloat(); break;
		case 2: // top 
			dest.y -= targ->range.getFloat(); break;
		case 3: // bottom 
			dest.y += targ->range.getFloat(); break;
	}
	
	shared_ptr<Room> ourRoom = world.map.roomAt(src.x, src.y);
	if (!ourRoom) {
		// (should we REALLY check for it being in the room system, here?)
		cerr << agent->identify() << " tried using GRID but isn't in the room system!\n";
		result.setInt(-1);
		return;
	}

	unsigned int dummy1; Line dummy2; Point point; shared_ptr<Room> room;
	bool collided = world.map.collideLineWithRoomBoundaries(src, dest, ourRoom, room, point, dummy2, dummy1, targ->perm);

	if (!room) result.setInt(-1);
	else result.setInt(room->id);
}

/**
 EMIT (command) caindex (integer) amount (float)
 %status maybe

 Makes the TARG agent continually emit the specified amount of the specified CA into the room.
*/
void caosVM::c_EMIT() {
	VM_VERIFY_SIZE(2)
	VM_PARAM_FLOAT(amount)
	VM_PARAM_INTEGER(caindex)
	
	caos_assert((0 <= caindex && caindex <= 19) || caindex == -1);
	valid_agent(targ);

	targ->emitca_index = caindex;
	targ->emitca_amount = amount;
}

/**
 WALL (integer)
 %status maybe
 %pragma variants c2 cv c3

 Returns the direction of the last wall the TARG agent collided with.
*/
void caosVM::v_WALL() {
	VM_VERIFY_SIZE(0)

	valid_agent(targ);
	result.setInt(targ->lastcollidedirection);
}

/**
 ALTR (command) roomid (integer) caindex (integer) delta (float)
 %status maybe

 Modifies the level of the given CA in the room specified.
 If 'roomid' is -1, the room containing the TARG agent will be used.
*/
void caosVM::c_ALTR() {
	VM_VERIFY_SIZE(3)
	VM_PARAM_FLOAT(delta);
	VM_PARAM_INTEGER(caindex);
	VM_PARAM_INTEGER(roomid);
	
	caos_assert(0 <= caindex && caindex <= 19);

	shared_ptr<Room> room;
	if (roomid == -1) {
		valid_agent(targ);
		room = world.map.roomAt(targ->x + (targ->getWidth() / 2.0f), targ->y + (targ->getHeight() / 2.0f));
	} else
		room = world.map.getRoom(roomid);
	caos_assert(room);
	float newvalue = room->ca[caindex] + delta;
	if (newvalue < 0.0f) newvalue = 0.0f;
	else if (newvalue > 1.0f) newvalue = 1.0f;
	room->ca[caindex] = newvalue;
}

/**
 RLOC (string) roomid (integer)
 %status maybe

 Returns a string containing the location of the given room in the following format: x_left, x_right, y_left_ceiling, 
 y_right_ceiling, y_left_floor, y_right_floor.
*/
void caosVM::v_RLOC() {
	VM_PARAM_INTEGER(roomid)

	shared_ptr<Room> r = world.map.getRoom(roomid);
	caos_assert(r);

	result.setString(boost::str(boost::format("%d %d %d %d %d %d") % r->x_left % r->x_right % r->y_left_ceiling % r->y_right_ceiling % r->y_left_floor % r->y_right_floor));
}

/**
 MLOC (string) metaroomid (integer)
 %status maybe

 Returns a string containing the location of the given metaroom in the following format: x y width height
*/
void caosVM::v_MLOC() {
	VM_PARAM_INTEGER(metaroomid)

	MetaRoom *r = world.map.getMetaRoom(metaroomid);
	caos_assert(r);

	result.setString(boost::str(boost::format("%d %d %d %d") % r->x() % r->y() % r->width() % r->height()));
}

/**
 DMAP (command) mapon (integer)
 %status maybe

 Turns the debug map on and off, which shows the edges of rooms and vehicles.
*/
void caosVM::c_DMAP() {
	VM_PARAM_INTEGER(mapon)
		
	world.showrooms = mapon;
}

/**
 SYS: DMAP (command) mapon (integer)
 %status maybe
 %pragma variants c2
*/
void caosVM::c_SYS_DMAP() {
	c_DMAP();
}

/**
 ERID (string) metaroom_id (integer)
 %status maybe

 Returns a space-seperated list of all room id's contained by the given metaroom.
*/
void caosVM::v_ERID() {
	VM_PARAM_INTEGER(metaroom_id)

	std::string out;

	if (metaroom_id == -1) {
		// TODO
	} else {
		MetaRoom *r = world.map.getMetaRoom(metaroom_id);
		for (std::vector<shared_ptr<Room> >::iterator i = r->rooms.begin(); i != r->rooms.end(); i++) {
			if (out.size() > 0) out = out + " ";
			out = out + boost::str(boost::format("%d") % (*i)->id);
		}
	}

	result.setString(out);
}

/**
 DELR (command) room_id (integer)
 %status stub
 %pragma variants c2 cv c3

 Removes the given room from the map.
*/
void caosVM::c_DELR() {
	VM_PARAM_INTEGER(room_id)

	shared_ptr<Room> r = world.map.getRoom(room_id);
	caos_assert(r);

	// TODO
}

/**
 DELM (command) metaroom_id (integer)
 %status stub

 Removes the given metaroom from the map.
*/
void caosVM::c_DELM() {
	VM_PARAM_INTEGER(metaroom_id)

	MetaRoom *r = world.map.getMetaRoom(metaroom_id);
	caos_assert(r);

	// TODO
}

/**
 HIRP (integer) roomid (integer) caindex (integer) direction (integer)
 %status stub
*/
void caosVM::v_HIRP() {
	VM_PARAM_INTEGER(direction)
	VM_PARAM_INTEGER(caindex) caos_assert(0 <= caindex && caindex <= 19);
	VM_PARAM_INTEGER(roomid)
	
	shared_ptr<Room> r = world.map.getRoom(roomid);
	caos_assert(r);

	result.setInt(roomid); // TODO
}

/**
 LORP (integer) roomid (integer) caindex (integer) direction (integer)
 %status stub
*/
void caosVM::v_LORP() {
	VM_PARAM_INTEGER(direction)
	VM_PARAM_INTEGER(caindex) caos_assert(0 <= caindex && caindex <= 19);
	VM_PARAM_INTEGER(roomid)
	
	shared_ptr<Room> r = world.map.getRoom(roomid);
	caos_assert(r);

	result.setInt(roomid); // TODO
}

/**
 TORX (float) roomid (integer)
 %status maybe
*/
void caosVM::v_TORX() {
	VM_PARAM_INTEGER(roomid)

	shared_ptr<Room> r = world.map.getRoom(roomid);
	caos_assert(r);
	valid_agent(targ);

	float centrex = r->x_left + ((r->x_right - r->x_left) / 2.0f);
	result.setFloat(centrex - targ->x);
}

/**
 TORY (float) roomid (integer)
 %status maybe
*/
void caosVM::v_TORY() {
	VM_PARAM_INTEGER(roomid)

	shared_ptr<Room> r = world.map.getRoom(roomid);
	caos_assert(r);
	valid_agent(targ);

	// TODO: calculate this however c2e does it.. or at least check this is right
	float topy = (r->y_left_ceiling - r->y_right_ceiling) / 2.0f;
	if (topy >= 0.0f) topy = r->y_left_ceiling + topy;
	else topy = r->y_right_ceiling - topy;

	float bottomy = (r->y_left_floor - r->y_right_floor) / 2.0f;
	if (bottomy >= 0.0f) topy = r->y_left_floor + bottomy;
	else bottomy = r->y_right_floor - bottomy;

	float centrey = topy + ((bottomy - topy) / 2.0f);
	result.setFloat(centrey - targ->y);
}

/**
 CACL (command) family (integer) genus (integer) species (integer) caindex (integer)
 %status stub
*/
void caosVM::c_CACL() {
	VM_PARAM_INTEGER(caindex) caos_assert(0 <= caindex && caindex <= 19);
	VM_PARAM_INTEGER(species) caos_assert(0 <= species && species <= 65535);
	VM_PARAM_INTEGER(genus) caos_assert(0 <= genus && genus <= 255);
	VM_PARAM_INTEGER(family) caos_assert(0 <= family && family <= 255);

	// TODO
}

/**
 WIND (integer)
 %status stub
 %pragma variants c1

 Always returns zero, since this command was stubbed in C1.
*/
void caosVM::v_WIND() {
	result.setInt(0);
}

/**
 TEMP (variable)
 %status maybe
 %pragma variants c1 c2
*/
void caosVM::v_TEMP() {
	// TODO: should this be a separate "return 0 always" stub for C1?

	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	vm->valueStack.push_back(&r->temp);
}

/**
 LITE (variable)
 %status maybe
 %pragma variants c2
*/
void caosVM::v_LITE() {
	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	vm->valueStack.push_back(&r->lite);
}

/**
 RADN (variable)
 %status maybe
 %pragma variants c2
*/
void caosVM::v_RADN() {
	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	vm->valueStack.push_back(&r->radn);
}

/**
 ONTR (variable)
 %status maybe
 %pragma variants c2
*/
void caosVM::v_ONTR() {
	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	vm->valueStack.push_back(&r->ontr);
}

/**
 INTR (variable)
 %status maybe
 %pragma variants c2
*/
void caosVM::v_INTR() {
	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	vm->valueStack.push_back(&r->intr);
}

/**
 PRES (variable)
 %status maybe
 %pragma variants c2
*/
void caosVM::v_PRES() {
	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	vm->valueStack.push_back(&r->pres);
}

/**
 HSRC (variable)
 %status maybe
 %pragma variants c2
*/
void caosVM::v_HSRC() {
	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	vm->valueStack.push_back(&r->hsrc);
}

/**
 LSRC (variable)
 %status maybe
 %pragma variants c2
*/
void caosVM::v_LSRC() {
	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	vm->valueStack.push_back(&r->lsrc);
}

/**
 RSRC (variable)
 %status maybe
 %pragma variants c2
*/
void caosVM::v_RSRC() {
	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	vm->valueStack.push_back(&r->rsrc);
}

/**
 PSRC (variable)
 %status maybe
 %pragma variants c2
*/
void caosVM::v_PSRC() {
	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	vm->valueStack.push_back(&r->psrc);
}

/**
 WNDX (integer)
 %status maybe
 %pragma variants c2
*/
void caosVM::v_WNDX() {
	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	result.setInt(r->windx);
}

/**
 WNDY (integer)
 %status maybe
 %pragma variants c2
*/
void caosVM::v_WNDY() {
	valid_agent(targ);
	shared_ptr<Room> r = roomContainingAgent(targ);
	caos_assert(r);
	result.setInt(r->windy);
}

/**
 DOCA (command) times (integer)
 %status stub
*/
void caosVM::c_DOCA() {
	VM_PARAM_INTEGER(times)

	// TODO
}

/**
 SETV DOOR (command) direction (integer) room1 (integer) room2 (integer) value (integer)
 %status maybe
 %pragma variants c2
*/
void caosVM::c_SETV_DOOR() {
	VM_PARAM_INTEGER(value)
	VM_PARAM_INTEGER(room2)
	VM_PARAM_INTEGER(room1)
	VM_PARAM_INTEGER(perm)

	// TODO: what's direction for?

	// code identical to c2e DOOR
	shared_ptr<Room> r1 = world.map.getRoom(room1);
	shared_ptr<Room> r2 = world.map.getRoom(room2);
	caos_assert(r1); caos_assert(r2);
	if (r1->doors.find(r2) == r1->doors.end()) {
		RoomDoor *door = new RoomDoor;
		door->first = r1;
		door->second = r2;
		door->perm = perm;
		r1->doors[r2] = door;
		r2->doors[r1] = door;
	} else {
		RoomDoor *door = r1->doors[r2];
		door->perm = perm;
	}
}

/**
 FLOR (integer)
 %status stub
 %pragma variants c2

 Return y coordinate of floor below centre of target agent.
*/
void caosVM::v_FLOR() {
	valid_agent(targ);

	result.setInt(0); // TODO
}

/**
 GNDW (integer)
 %status stub
 %pragma variants c1

 Return the number of horizontal pixels per piece of ground level data.
*/
void caosVM::v_GNDW() {
	result.setInt(4); // TODO: is it always 4? :)
}

/**
 GRND (integer) index (integer)
 %status stub
 %pragma variants c1

 Return the ground level data at the provided index. See GNDW to work out the index required.
*/
void caosVM::v_GRND() {
	VM_PARAM_INTEGER(index)

	result.setInt(0); // TODO
}

/**
 ROOM (command) roomno (integer) left (integer) top (integer) right (integer) bottom (integer) type (integer)
 %status maybe
 %pragma variants c1

 Create or modify a room.
*/
void caosVM::c_ROOM() {
	VM_PARAM_INTEGER(type)
	VM_PARAM_INTEGER(bottom)
	VM_PARAM_INTEGER(right)
	VM_PARAM_INTEGER(top)
	VM_PARAM_INTEGER(left)
	VM_PARAM_INTEGER(roomno)

	shared_ptr<Room> r = world.map.getRoom(roomno);
	if (!r) {
		shared_ptr<Room> r2(new Room(left, right, top, top, bottom, bottom));
		r = r2;

		MetaRoom *m = world.map.getMetaRoom(0);
		unsigned int roomid = m->addRoom(r);
		assert(roomid == (unsigned int)roomno); // TODO: this is fairly likely to fail, but is a major bug if it does, FIX ME!
	} else {
		r->x_left = left;
		r->x_right = right;
		r->y_left_ceiling = r->y_right_ceiling = top;
		r->y_left_floor = r->y_right_floor = bottom;
	}

	r->type.setInt(type);
}

/**
 ROOM (integer) roomno (integer) data (integer)
 %status maybe
 %pragma variants c1
 %pragma implementation caosVM::v_ROOM_c1

 Return some data for the specified room number. Data of 0 is left, 1 is right, 2 is top, 3 is bottom, 4 is room type. Returns 0 if no such room.
*/
void caosVM::v_ROOM_c1() {
	VM_PARAM_INTEGER(data) caos_assert(data >= 0 && data <= 4);
	VM_PARAM_INTEGER(roomno)

	shared_ptr<Room> r = world.map.getRoom(data);
	if (!r) {
		result.setInt(0);
		return;
	}

	switch (data) {
		case 0:
			result.setInt(r->x_left);
			break;

		case 1:
			result.setInt(r->x_right);
			break;

		case 2:
			result.setInt(r->y_left_ceiling);
			break;

		case 3:
			result.setInt(r->y_left_floor);
			break;

		case 4:
			result.setInt(r->type.getInt());
			break;
	}
}

/* vim: set noet: */
