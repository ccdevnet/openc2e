/*
 *  PointerAgent.h
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

#include "SimpleAgent.h"
#include "Backend.h"

#ifndef _C2E_POINTERAGENT_H
#define _C2E_POINTERAGENT_H

class PointerAgent : public SimpleAgent {
protected:
	int hotspotx, hotspoty;

public:
	std::string name;
	bool handle_events;

	int holdingWire; // 0 for no, 1 for from output port, 2 for from input port
	AgentRef wireOriginAgent;
	unsigned int wireOriginID;

	PointerAgent(std::string spritefile);
	void finishInit();
	void firePointerScript(unsigned short event, Agent *src);
	void physicsTick();
	void kill();
	void handleEvent(SomeEvent &event);
	void setHotspot(int, int);
};

#endif
/* vim: set noet: */
