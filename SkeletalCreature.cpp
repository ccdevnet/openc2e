/*
 *  SkeletalCreature.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Thu 10 Mar 2005.
 *  Copyright (c) 2005-2006 Alyssa Milburn. All rights reserved.
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
/* 20th feb 2005, by fuzzie
   with thanks to vadim for helping out

   TODO:
	* hair tidiness/untidiness (offset by 0 to 2 multiplied by 16)
	* render hairs/ears .. at the moment we avoid them, we lack zorder sanity too
*/

#include "SkeletalCreature.h"
#include "Creature.h"
#include "World.h"
#include "Engine.h"
#include "Backend.h"

#include <typeinfo> // TODO: remove when genome system is fixed
#include <boost/format.hpp>

struct bodypartinfo {
	char letter;
	int parent;
	int attorder;
};

bodypartinfo cee_bodyparts[17] = {
	{ 'b', -1, -1 }, // body
	{ 'a', 0, 0 }, // head - attached to body
	{ 'c', 0, 1 }, // left thigh - attached to body
	{ 'f', 0, 2 }, // right thigh - attached to body
	{ 'i', 0, 3 }, // left humerus - attached to body
	{ 'k', 0, 4 }, // right humerus - attached to body
	{ 'm', 0, 5 }, // tail root - attached to body
	{ 'd', 2, 1 }, // left shin - attached to left thigh
	{ 'g', 3, 1 }, // right shin - attached to right thigh
	{ 'j', 4, 1 }, // left radius - attached to left humerus
	{ 'l', 5, 1 }, // right radius - attached to right humerus
	{ 'e', 7, 1 }, // left foot - attached to left shin
	{ 'h', 8, 1 }, // right foot - attached to right shin
	{ 'n', 6, 1 }, // tail tip - attached to tail root
	// Creatures Village only:
	{ 'o', 1, -1 }, // left ear - attached to head
	{ 'p', 1, -1 }, // right ear - attached to head
	{ 'q', 1, -1 } // hair - attached to head
};

unsigned int cee_zorder[4][14] = {
	{ 6, 13, 2, 7, 11, 4, 9, 0, 1, 5, 3, 8, 10, 12 },
	{ 6, 13, 3, 8, 12, 5, 10, 0, 1, 2, 7, 11, 4, 9 },
	{ 6, 13, 2, 4, 9, 5, 3, 7, 8, 10, 0, 11, 12, 1 },
	{ 2, 4, 9, 1, 5, 3, 7, 8, 10, 11, 12, 0, 6, 13 }
};

// needed for setPose(string) at least .. maybe cee_bodyparts should be indexed by letter
unsigned int cee_lookup[17] = { 1, 0, 2, 7, 11, 3, 8, 12, 4, 9, 5, 10, 6, 13, 14, 15, 16 };

std::string SkeletalCreature::dataString(unsigned int _stage, bool sprite, unsigned int dataspecies, unsigned int databreed) {
	// TODO: dataspecies is nonsense in c1
	char _postfix[4] = "XXX";
	_postfix[0] = '0' + dataspecies + ((sprite && creature->isFemale()) ? 4 : 0);
	_postfix[1] = '0' + _stage;
	if (engine.version == 1)
		_postfix[2] = '0' + databreed;
	else
		_postfix[2] = 'a' + databreed;
	return _postfix;
}

SkeletalCreature::SkeletalCreature(unsigned char _family, Creature *c) : CreatureAgent(_family, c) {
	facialexpression = 0;
	pregnancy = 0;
	eyesclosed = false;

	for (int i = 0; i < 14; i++) {
		images[i] = gallery_p();
	}
	
	skeletonInit();

	// needs to go last for now, so we can throw exceptions from skeletonInit
	skeleton = new SkeletonPart(this);
}

void SkeletalCreature::skeletonInit() {
	//TODO: the exception throwing in here needs some more thought

	creatureAppearanceGene *appearance[5] = { 0, 0, 0, 0, 0 };
	for (vector<gene *>::iterator i = creature->getGenome()->genes.begin(); i != creature->getGenome()->genes.end(); i++) {
		if (typeid(*(*i)) == typeid(creatureAppearanceGene)) {
			creatureAppearanceGene *x = (creatureAppearanceGene *)(*i);
			if (x->part > 4)
				throw creaturesException(boost::str(boost::format("SkeletalCreature didn't understand a gene with a part# of %d") % (int)x->part));
			if (appearance[x->part])
				throw creaturesException(boost::str(boost::format("SkeletalCreature got a duplicated gene for part# %d") % (int)x->part));
			appearance[x->part] = x;
		}
	}

	for (int i = 0; i < 14; i++) {
		if (engine.version == 1) // TODO: this is hackery to skip tails for C1
			if (i == 6 || i == 13) continue;

		// try this stage and the stages below it to find data which worksforus
		images[i] = gallery_p();
		char x = cee_bodyparts[i].letter;
		int stage_to_try = creature->getStage();
		creatureAppearanceGene *partapp = 0;
		if (x == 'a' || x >= 'o') {
			// head
			partapp = appearance[0];
		} else if (x == 'b') {
			// body
			partapp = appearance[1];
		} else if (x >= 'c' && x <= 'h') {
			// legs
			partapp = appearance[2];
		} else if (x >= 'i' && x <= 'm') {
			// arms
			partapp = appearance[3];
		} else if (x == 'n') {
			// tail
			partapp = appearance[4];
		}
		// TODO: this exception won't necessary be handled, neither will the one below
		if (!partapp)
			throw creaturesException(boost::str(boost::format("SkeletalCreature doesn't understand appearance id '%c'") % (unsigned char)x));
	
		if (engine.version == 1) partapp->species = 0; // TODO: don't stomp over the gene? :P
		while (stage_to_try > -1 && images[i] == 0) {
			images[i] = world.gallery.getGallery(x + dataString(stage_to_try, true, partapp->species, partapp->variant));
			if (images[i] == 0) stage_to_try--;
		}
		if (images[i] == 0)
			throw creaturesException(boost::str(boost::format("SkeletalCreature couldn't find an image for species %d, variant %d, stage %d") % (int)partapp->species % (int)partapp->variant % (int)creature->getStage()));

		std::string attfilename = world.findFile(std::string("/Body Data/") + x + dataString(stage_to_try, false, partapp->species, partapp->variant) + ".att");
		if (attfilename.empty())
			throw creaturesException(boost::str(boost::format("SkeletalCreature couldn't find body data for species %d, variant %d, stage %d") % (int)partapp->species % (int)partapp->variant % stage_to_try));
		std::ifstream in(attfilename.c_str());
		if (in.fail())
			throw creaturesException(boost::str(boost::format("SkeletalCreature couldn't load body data for species %d, variant %d, stage %d") % (int)partapp->species % (int)partapp->variant % stage_to_try));
		in >> att[i];
	}

	setPose(0);
}

SkeletalCreature::~SkeletalCreature() {
	delete skeleton;
}

void SkeletalCreature::render(Surface *renderer, int xoffset, int yoffset) {
	for (int j = 0; j < 14; j++) {
		int i = cee_zorder[direction][j];

		if (engine.version == 1) // TODO: this is hackery to skip tails for C1
			if (i == 6 || i == 13) continue;

		bodypartinfo *part = &cee_bodyparts[i];

		unsigned int ourpose;
		if (part->parent == -1) // body
			ourpose = pose[i] + (pregnancy * 16);
		else if (i == 1) // head
			ourpose = pose[i] + (eyesclosed ? 16 : 0) + (facialexpression * 32);
		else // everything else
			ourpose = pose[i];

		assert(images[i]);

		renderer->render(images[i]->getSprite(ourpose), partx[i] + adjustx + xoffset, party[i] + adjusty + yoffset, false, 0);

		// if (displaycore) {
			// TODO: we draw a lot of points twice here :)
			int atx = attachmentX(i, 0) + xoffset, aty = attachmentY(i, 0) + yoffset;
			renderer->renderLine(atx - 1, aty, atx + 1, aty, 0xFF0000CC);
			renderer->renderLine(atx, aty - 1, atx, aty + 1, 0xFF0000CC);
			atx = attachmentX(i, 1) + xoffset; aty = attachmentY(i, 1) + yoffset;
			renderer->renderLine(atx - 1, aty, atx + 1, aty, 0xFF0000CC);
			renderer->renderLine(atx, aty - 1, atx, aty + 1, 0xFF0000CC);
		// }
	}
}

int SkeletalCreature::attachmentX(unsigned int part, unsigned int id) {
	return partx[part] + att[part].attachments[pose[part]][0 + (id * 2)] + adjustx;
}

int SkeletalCreature::attachmentY(unsigned int part, unsigned int id) {
	return party[part] + att[part].attachments[pose[part]][1 + (id * 2)] + adjusty;
}

void SkeletalCreature::recalculateSkeleton() {
	int lowestx = 0, lowesty = 0, highestx = 0, highesty = 0;

	for (int i = 0; i < 14; i++) {
		if (engine.version == 1) // TODO: this is hackery to skip tails for C1
			if (i == 6 || i == 13) continue;

		bodypartinfo *part = &cee_bodyparts[i];

		if (part->parent == -1) {
			partx[i] = 0; party[i] = 0;
		} else {
			attFile &bodyattinfo = att[0];
			attFile &attinfo = att[i];

			int attachx = att[i].attachments[pose[i]][0];
			int attachy = att[i].attachments[pose[i]][1];
			int x, y;

			if (part->parent == 0) { // linking to body
				x = bodyattinfo.attachments[pose[0]][part->attorder * 2];
				y = bodyattinfo.attachments[pose[0]][(part->attorder * 2) + 1];
			} else { // extra limb
				attFile &parentattinfo = att[part->parent];

				x = partx[part->parent] + parentattinfo.attachments[pose[part->parent]][part->attorder * 2]; 
				y = party[part->parent] + parentattinfo.attachments[pose[part->parent]][(part->attorder * 2) + 1];
			}

			x = x - attachx;
			y = y - attachy;

			partx[i] = x; party[i] = y;

			if (x < lowestx) { lowestx = x; }
			if (y < lowesty) { lowesty = y; }
			if (x + (int)images[i]->width(pose[i]) > highestx) { highestx = x + images[i]->width(pose[i]); }
			if (y + (int)images[i]->height(pose[i]) > highesty) { highesty = y + images[i]->height(pose[i]); }
		}
	}

	adjustx = -lowestx;
	adjusty = -lowesty;
	//width = highestx - lowestx;
	width = 50;
	//height = highesty - lowesty;
	int leftfoot = party[11] + att[11].attachments[pose[11]][3];
	int rightfoot = party[12] + att[12].attachments[pose[12]][3];
	height = (leftfoot < rightfoot ? rightfoot : leftfoot) - lowesty;
}
	
void SkeletalCreature::setPose(unsigned int p) {
	direction = 0;
	for (int i = 0; i < 14; i++)
		pose[i] = p;
	recalculateSkeleton();
}

void SkeletalCreature::setPose(std::string s) {
	switch (s[0]) {
		case '?': direction = 0; break; // hack
		case '!': direction = 1; break; // hack
		case '0': direction = 3; break;
		case '1': direction = 2; break;
		case '2': direction = 0; break;
		case '3': direction = 1; break;
		default: assert(false);
	}

	for (int i = 0; i < 14; i++) {
		switch (s[i + 1]) {
			case '0': pose[cee_lookup[i]] = 0 + (direction * 4); break;
			case '1': pose[cee_lookup[i]] = 1 + (direction * 4); break;
			case '2': pose[cee_lookup[i]] = 2 + (direction * 4); break;
			case '3': pose[cee_lookup[i]] = 3 + (direction * 4); break;
			case '?': assert(i == 0); pose[1] = 0 + (direction * 4); break; // hack
			case 'X': break; // do nothing
			default: assert(false); 
		}
	}
		
	recalculateSkeleton();
}

void SkeletalCreature::setPoseGene(unsigned int poseno) {
	/* TODO: this sets by sequence, now, not the 'poseno' inside the gene.
	 * this is what the POSE caos command does. is this right? - fuzzie */
	creaturePoseGene *g = (creaturePoseGene *)creature->getGenome()->getGene(2, 3, poseno);
	assert(g); // TODO: -> caos_assert

	gaitgene = 0;
}

void SkeletalCreature::setGaitGene(unsigned int gaitdrive) { // TODO: not sure if this is *useful*
	for (vector<gene *>::iterator i = creature->getGenome()->genes.begin(); i != creature->getGenome()->genes.end(); i++) {
		if (typeid(*(*i)) == typeid(creatureGaitGene)) {
			creatureGaitGene *g = (creatureGaitGene *)(*i);
			if (g->drive == gaitdrive) {
				gaitgene = g;
				gaiti = 0;
			}
		}
	}

	// explode!
	gaitgene = 0;
	gaitTick();
}

void SkeletalCreature::gaitTick() {
	if (!gaitgene) return;
	uint8 pose = gaitgene->pose[gaiti];
	creaturePoseGene *poseg = 0;
	for (vector<gene *>::iterator i = creature->getGenome()->genes.begin(); i != creature->getGenome()->genes.end(); i++) {
		if (typeid(*(*i)) == typeid(creaturePoseGene)) {
			creaturePoseGene *g = (creaturePoseGene *)(*i);
			if (g->poseno == pose)
				poseg = g;
			
		}
	}
	assert(poseg); // TODO: don't assert. caos_assert? but this means a bad genome file, always.
	setPose(poseg->getPoseString());
	gaiti++; if (gaiti > 7) gaiti = 0;
}

CompoundPart *SkeletalCreature::part(unsigned int id) {
	return skeleton;
}

void SkeletalCreature::setZOrder(unsigned int plane) {
	Agent::setZOrder(plane);
	skeleton->zapZOrder();
	skeleton->addZOrder();
}

SkeletonPart::SkeletonPart(SkeletalCreature *p) : CompoundPart(p, 0, 0, 0, 0) {
}

void SkeletonPart::tick() {
}

void SkeletonPart::partRender(class Surface *renderer, int xoffset, int yoffset) {
	SkeletalCreature *c = dynamic_cast<SkeletalCreature *>(parent);
	c->render(renderer, xoffset, yoffset);	
}

void SkeletalCreature::creatureAged() {
	skeletonInit();
}

std::string SkeletalCreature::getFaceSpriteName() {
	for (vector<gene *>::iterator i = creature->getGenome()->genes.begin(); i != creature->getGenome()->genes.end(); i++) {
		if (typeid(*(*i)) == typeid(creatureAppearanceGene)) {
			creatureAppearanceGene *x = (creatureAppearanceGene *)(*i);
			if (x->part == 0) {
				return std::string("a") + dataString(0, true, x->species, x->variant);
			}
		}
	}

	caos_assert(false); // TODO: mmh
}

unsigned int SkeletalCreature::getFaceSpriteFrame() {
	return 9 + (eyesclosed ? 16 : 0) + (facialexpression * 32);
}

/* vim: set noet: */
