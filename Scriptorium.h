/*
 *  Scriptorium.h
 *  openc2e
 *
 *  Created by Alyssa Milburn on 05/11/2004.
 *  Copyright 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include "caosScript.h"
#include <map>

class Scriptorium {
protected:
	// unsigned int = combined family/genus/species
	// unsigned short = event id
	std::map<unsigned int, std::map<unsigned short, script *> > scripts;
	
	std::map<unsigned short, script *> &getScripts(unsigned int value) { return scripts[value]; }
	unsigned int calculateValue(unsigned char family, unsigned char genus, unsigned short species);

public:
	void addScript(unsigned char family, unsigned char genus, unsigned short species, unsigned short event, script *s);
	void delScript(unsigned char family, unsigned char genus, unsigned short species, unsigned short event);
	script *getScript(unsigned char family, unsigned char genus, unsigned short species, unsigned short event);
};
/* vim: noet : */
