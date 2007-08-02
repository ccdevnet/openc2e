/*
 *  caosVM.h
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

#ifndef CAOSSCRIPT_H
#define CAOSSCRIPT_H

#include <vector>
#include <list>
#include <string>
#include <istream>
#include <map>
#include "caosVar.h"
#include <cassert>
#include "openc2e.h"
#include "bytecode.h"
#include "caosVar.h"
#include "dialect.h"


class Agent;

struct script {
	protected:
		FRIEND_SERIALIZE(script);
		
		bool linked;

		// position 0 is reserved in the below vector
		std::vector<int> relocations;
		script() {}
	public:
		// pos-0 needs to be initted to a caosNoop <-- still needed?
		std::vector<caosOp> ops;
		std::vector<caosVar> consts;

	public:
		int fmly, gnus, spcs, scrp;
		const class Dialect *dialect;
		const Dialect *getDialect() const { return dialect; };
		
		std::string filename;

		caosOp getOp(int idx) const {
			assert (idx >= 0);
			return (unsigned int)idx >= ops.size() ? caosOp(CAOS_DIE, -1) : ops[idx];
		}

		int scriptLength() const {
			return ops.size();
		}

		caosVar getConstant(int idx) const {
			if (idx < 0 || (size_t)idx >= consts.size()) {
				caosVar v;
				v.reset();
				return v;
			}
			return consts[idx];
		}
		
		std::map<std::string, int> gsub;
		int getNextIndex() { return ops.size(); }
		// add op as the next opcode
		void thread(caosOp *op);
		script(const Dialect *v, const std::string &fn,
				int fmly_, int gnus_, int spcs_, int scrp_);
		script(const Dialect *v, const std::string &fn);
		~script();
		std::string dump();
	//	std::string dumpLine(unsigned int);

		void link();
		
		int newRelocation() {
			assert (!linked);
			int idx = relocations.size();
			relocations.push_back(0);
			return -idx;
		}

		// fix relocation r to point to the next op to be emitted
		// XXX: maybe make relocations lightweight classes, so we
		// can identify leaks.

		void fixRelocation(int r, int p) {
			assert (!linked);
			assert (r < 0);
			r = -r;
			assert (relocations[r] == 0);
			// check for a loop
			int i = p;
			while (i < 0) {
				if (i == r)
					throw creaturesException("relocation loop found");
				i = relocations[-i];
			}
			
			relocations[r] = p;
		}
			
		void fixRelocation(int r) {
			fixRelocation(r, getNextIndex());
		}

		
};

class caosScript { //: Collectable {
public:
	const Dialect *d;
	std::string filename;
	shared_ptr<script> installer, removal;
	std::vector<shared_ptr<script> > scripts;
	shared_ptr<script> current;

	caosScript(const std::string &dialect, const std::string &fn);
	caosScript() { d = NULL; }
	void parse(std::istream &in);
	~caosScript();
	void installScripts();
	void installInstallScript(unsigned char family, unsigned char genus, unsigned short species, unsigned short eventid);
protected:
	static int readCond();
	void parseCondition();
	void emitOp(opcode_t op, int argument);
	void readExpr(const enum ci_type *argp);
	const cmdinfo *readCommand(class token *t, const std::string &prefix);
	void parseloop(int state, void *info);
};

#endif
/* vim: set noet: */
