/*
 *  Lift.h
 *  openc2e
 *
 *  Created by Alyssa Milburn on Sat Dec 2 2006.
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

#include "Vehicle.h"

#ifndef _C2E_LIFT_H
#define _C2E_LIFT_H

class Lift : public Vehicle {
public:
	unsigned int currentbutton;
	std::vector<int> callbuttony;

	Lift(unsigned char family, unsigned char genus, unsigned short species, unsigned int plane,
		std::string spritefile, unsigned int firstimage, unsigned int imagecount)
		: Vehicle(family, genus, species, plane, spritefile, firstimage, imagecount) { }
	void tick();
	bool fireScript(unsigned short event, Agent *from);
};

#endif
/* vim: set noet: */
