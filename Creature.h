/*
 *  Creature.h
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
#include "genome.h"
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

class Creature;

struct Reaction {
	bioReaction *data;
	float rate;
	unsigned int receptors;
	void init(bioReaction *);
};

struct Receptor {
	bioReceptor *data;
	bool processed;
	float lastvalue;
	float *locus;
	unsigned int *receptors;
	float nominal, threshold, gain;
	void init(bioReceptor *, class Organ *, shared_ptr<Reaction>);
};

struct Emitter {
	bioEmitter *data;
	unsigned char sampletick;
	float *locus;
	float threshold, gain;
	void init(bioEmitter *);
};

class Organ {
protected:
	friend struct Receptor;
	
	Creature *parent;	
	organGene *ourGene;

	std::vector<shared_ptr<Reaction> > reactions;
	std::vector<Receptor> receptors;
	std::vector<Emitter> emitters;

	// data
	float energycost, atpdamagecoefficient;
	
	// variables
	float lifeforce, shorttermlifeforce, longtermlifeforce;
	
	// locuses
	float biotick, damagerate, repairrate, clockrate, injurytoapply;
	unsigned int clockratereceptors, repairratereceptors, injuryreceptors;

	void applyInjury(float);

	void processReaction(Reaction &);
	void processEmitter(Emitter &);
	void processReceptor(Receptor &, bool checkchem);

public:
	Organ(Creature *p, organGene *g);

	float getEnergyCost() { return energycost; }
	void tick();
};

class Brain {
public:
};

class Creature : public Agent {
protected:
	std::vector<shared_ptr<Organ> > organs;
	Brain brain;

	// biochemistry
	float chemicals[256];

	// non-specific bits
	unsigned int variant;
	bool female;
	shared_ptr<genomeFile> genome;
	
	bool alive, asleep, dreaming, tickage;

	bool zombie;

	unsigned int age; // in ticks
	lifestage stage;

	AgentRef attention, focus;

	// clothes
	// linguistic stuff

	// drives
	// to-be-processed instincts
	// conscious flag? brain/motor enabled flags? flags for each 'faculty'?

	unsigned int biochemticks;

	void tickBiochemistry();

public:
	Creature(shared_ptr<genomeFile> g, unsigned char _family, bool is_female, unsigned char _variant);
	virtual ~Creature();
	void tick();

	virtual void ageCreature();
	lifestage getStage() { return stage; }
	void adjustChemical(unsigned char id, float value);
	float getChemical(unsigned char id) { return chemicals[id]; }
	unsigned int getVariant() { return variant; }
	void setAsleep(bool asleep);
	bool isAsleep() { return asleep; }
	void setDreaming(bool dreaming);
	bool isDreaming() { return dreaming; }
	bool isFemale() { return female; }
	bool isAlive() { return alive; }
	void setZombie(bool z) { zombie = z; }
	bool isZombie() { return zombie; }
	unsigned int getAge() { return age; }

	unsigned int noOrgans() { return organs.size(); }
	shared_ptr<Organ> getOrgan(unsigned int i) { return organs[i]; }
	
	void born();
	void die();
};

/* vim: set noet: */
