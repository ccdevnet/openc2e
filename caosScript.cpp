/*
 *  caosScript.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Wed May 26 2004.
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

#include "bytecode.h"
#include "cmddata.h"
#include "exceptions.h"
#include "caosVM.h"
#include "openc2e.h"
#include "World.h"
#include "token.h"
#include "dialect.h"
#include "lex.yy.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <boost/format.hpp>

using std::string;

class unexpectedEOIexception { };

script::~script() {
}

void script::link() {
	ops.push_back(caosOp(CAOS_STOP, 0));
	assert(!linked);
//	std::cout << "Pre-link:" << std::endl << dump();
	// check relocations
	for (unsigned int i = 1; i < relocations.size(); i++) {
		// handle relocations-to-relocations
		int p = relocations[i];
		while (p < 0)
			p = relocations[-p];
		relocations[i] = p;
	}
	for (unsigned int i = 0; i < ops.size(); i++) {
		if (op_is_relocatable(ops[i].opcode) && ops[i].argument < 0)
			ops[i].argument = relocations[ops[i].argument];
	}
	linked = true;
	relocations.clear();
//	std::cout << "Post-link:" << std::endl << dump();
}

script::script(const Dialect *v, const std::string &fn)
	: fmly(-1), gnus(-1), spcs(-1), scrp(-1),
	  dialect(v), filename(fn)
{
	// is this needed?
	ops.push_back(caosOp(CAOS_NOP, 0));
	relocations.push_back(0);
	linked = false;
}
	
script::script(const Dialect *v, const std::string &fn,
		int fmly_, int gnus_, int spcs_, int scrp_)
	: fmly(fmly_), gnus(gnus_), spcs(spcs_), scrp(scrp_),
	  dialect(v), filename(fn)
{
	// is this needed?
	ops.push_back(caosOp(CAOS_NOP, 0));
	relocations.push_back(0);
	linked = false;
}

std::string script::dump() {
	std::ostringstream oss;
	oss << "Relocations:" << std::endl;
	for (unsigned int i = 1; i < relocations.size(); i++) {
		oss << boost::format("%08d -> %08d") % i % relocations[i]
			<< std::endl;
	}
	oss << "Code:" << std::endl;
	for (unsigned int i = 0; i < ops.size(); i++) {
		oss << boost::format("%08d: ") % i;
		oss << dumpOp(ops[i]);
		oss << std::endl;
	}
	return oss.str();
}

caosScript::caosScript(const std::string &dialect, const std::string &fn) {
	d = dialects[dialect].get();
	if (!d)
		throw caosException(std::string("Unknown dialect ") + dialect);
	current = installer = shared_ptr<script> (new script(d, fn));
	filename = fn;
}

caosScript::~caosScript() {
	// Nothing to do, yay shared_ptr!
}

void caosScript::installScripts() {
	std::vector<shared_ptr<script> >::iterator i = scripts.begin();
	while (i != scripts.end()) {
		shared_ptr<script> s = *i;
		world.scriptorium.addScript(s->fmly, s->gnus, s->spcs, s->scrp, s);
		i++;
	}
}

void caosScript::installInstallScript(unsigned char family, unsigned char genus, unsigned short species, unsigned short eventid) {
	assert((d->name == "c1") || (d->name == "c2"));

	installer->fmly = family;
	installer->gnus = genus;
	installer->spcs = species;
	installer->scrp = eventid;
	
	world.scriptorium.addScript(installer->fmly, installer->gnus, installer->spcs, installer->scrp, installer);
}

// parser states
enum {
	ST_INSTALLER,
	ST_BODY,
	ST_REMOVAL,
	ST_IF,
	ST_ENUM,
	ST_LOOP,
	ST_INVALID
};

void caosScript::parse(std::istream &in) {
	// restart the token parser
	yyrestart(&in, ((d->name == "c1") || (d->name == "c2")));

	parseloop(ST_INSTALLER, NULL);

	installer->link();
	if (removal)
		removal->link();
	std::vector<shared_ptr<script> >::iterator i = scripts.begin();
	while (i != scripts.end()) {
		(*i)->link();
		i++;
	}
}

const cmdinfo *caosScript::readCommand(token *t, const std::string &prefix) {
	std::string fullname = prefix + t->word;
	const cmdinfo *ci = d->find_command(fullname.c_str());
	if (!ci)
		throw caosException(std::string("Can't find command ") + fullname);
	if (ci->argtypes && ci->argtypes[0] == CI_SUBCOMMAND)
		return readCommand(getToken(TOK_WORD), fullname + " ");
	return ci;
}

void caosScript::readExpr(const enum ci_type *argp) {
	// TODO: bytestring
	// TODO: typecheck
	while (*argp != CI_END) {
		token *t = getToken(ANYTOKEN);
		switch (t->type) {
			case EOI: throw caosException("Unexpected end of input");
			case TOK_CONST:
				{
					if (t->constval.getType() == INTEGER) {
						int val = t->constval.getInt();
						if (val >= -(1 << 24) && val < (1 << 24)) {
							current->ops.push_back(caosOp(CAOS_CONSTINT, val));
							break;
						}
					}
					current->consts.push_back(t->constval);
					current->ops.push_back(caosOp(CAOS_CONST, current->consts.size() - 1));
	expout:
					break;
				}
			case TOK_WORD:
				{
					if (t->word == "face") {
						// horrible hack :(
						if (*argp == CI_NUMERIC)
							t->word = "face int";
						else
							t->word = "face string";
					}
					const cmdinfo *ci = readCommand(t, std::string("expr "));
					if (ci->argc)
						readExpr(ci->argtypes);
					current->ops.push_back(caosOp(CAOS_CMD, d->cmd_index(ci)));
					break;
				}
			default: throw caosException("Unexpected token");
		}
		argp++;
	}
}

void caosScript::parseloop(int state, void *info) {
	token *t;
	while ((t = getToken(ANYTOKEN))) {
		if (t->type == EOI) {
			switch (state) {
				case ST_INSTALLER:
				case ST_BODY:
				case ST_REMOVAL:
					return;
				default:
					throw caosException("Unexpected end of input");
			}
		}
		if (t->type != TOK_WORD) {
			throw caosException("Unexpected non-word token");
		}
		if (t->word == "scrp") {
			if (state == ST_INSTALLER)
				state = ST_BODY;
			if (state != ST_BODY)
				throw caosException("Unexpected SCRP");
			throw caosException("TODO");
		} else if (t->word == "rscr") {
			if (state == ST_INSTALLER || state == ST_BODY)
				state = ST_REMOVAL;
			else
				throw caosException("Unexpected RSCR");
			current = removal = shared_ptr<script>(new script(d, "TODO"));
		} else {
			const cmdinfo *ci = readCommand(t, std::string("cmd "));
			if (ci->argc)
				readExpr(ci->argtypes);
			current->ops.push_back(caosOp(CAOS_CMD, d->cmd_index(ci)));
		}
	}
}
			

/* vim: set noet: */
