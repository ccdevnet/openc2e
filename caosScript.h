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
#include "token.h"


class Agent;

struct script {
	protected:
		FRIEND_SERIALIZE(script);
		
		bool linked;

		// position 0 is reserved in the below vector
		// relocations[-relocid] is the target address for relocation relocid
		// will be 0 if unresolved
		std::vector<int> relocations;
		// map of name -> (address|relocation)
		std::map<std::string, int> labels;
		script() {}
	public:
		// ops[0] is initted to a nop, as address 0 is reserved for a flag value
		// in the relocation vector
		std::vector<caosOp> ops;
		// table of all non-trivial constants in the script
		// small immediates integers are done with CAOS_CONSTINT
		// mostly for strings and floats
		std::vector<caosVar> consts;
		// because caosVar doesn't store bytestrings, we store them in a separate
		// table
		std::vector<bytestring_t> bytestrs;

	public:
		int fmly, gnus, spcs, scrp;
		const class Dialect *dialect;
		const Dialect *getDialect() const { return dialect; };
		
		std::string filename;

		caosOp getOp(int idx) const {
			assert (idx >= 0);
			return (size_t)idx >= ops.size() ? caosOp(CAOS_DIE, -1) : ops[idx];
		}

		int scriptLength() const {
			return ops.size();
		}

		caosVar getConstant(int idx) const {
			if (idx < 0 || (size_t)idx >= consts.size()) {
				caosVar v;
				v.reset();
				return v; // XXX throw
			}
			return consts[idx];
		}

		bytestring_t getBytestr(int idx) const {
			if (idx < 0 || (size_t)idx >= bytestrs.size())
				return bytestring_t(); // XXX throw
			return bytestrs[idx];
		}
		
		std::map<std::string, int> gsub;
		int getNextIndex() { return ops.size(); }
		// add op as the next opcode
		void thread(caosOp *op); // FIXME: vestigal code (more like KILLME)
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

		void fixRelocation(int r, int p) {
			assert (!linked);
			assert (r < 0);
			r = -r;
			assert (relocations[r] == 0);
			// check for a loop
			int i = p;
			while (i < 0) {
				i = -i;
				if (i == r)
					throw creaturesException("relocation loop found");
				i = relocations[i];
			}
			
			relocations[r] = p;
		}

		// fix relocation r to point to the next op to be emitted
		void fixRelocation(int r) {
			fixRelocation(r, getNextIndex());
		}

		// find the address of the given label, could be a relocation
		int getLabel(const std::string &label) {
			if (labels.find(label) == labels.end())
				labels[label] = newRelocation();
			return labels[label];
		}

		// fix a label to the end of the current op string
		void affixLabel(const std::string &label) {
			int reloc = getLabel(label);
			if (reloc > 0)
				throw caosException(std::string("Label ") + label + " redefined");
			fixRelocation(reloc);
			labels[label] = getNextIndex();
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
	int readCond();
	void parseCondition();
	void emitOp(opcode_t op, int argument);
	void readExpr(const enum ci_type *argp);
	const cmdinfo *readCommand(class token *t, const std::string &prefix);
	void parseloop(int state, void *info);

	shared_ptr<std::vector<token> > tokens;
	int curindex; // index to the next token to be read
   	int errindex; // index to the token to report parse errors on
	int traceindex; // index to the token to report runtime errors on
	// deprecated support functions
	token *tokenPeek();
	void putBackToken(token *);
	token *getToken(toktype expected = ANYTOKEN);
};

#endif
/* vim: set noet: */
