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
			ops[i].argument = relocations[-ops[i].argument];
	}
	linked = true;
//	std::cout << "Post-link:" << std::endl << dump();
	relocations.clear();
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
	ST_DOIF,
	ST_ENUM,
	ST_LOOP,
	ST_REPS,
	ST_INVALID
};

struct doifinfo {
	int failreloc;
	int donereloc;
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

void caosScript::emitOp(opcode_t op, int argument) {
	current->ops.push_back(caosOp(op, argument));
}

void caosScript::readExpr(const enum ci_type *argp) {
	// TODO: bytestring
	// TODO: typecheck
	if (!argp) throw caosException("Internal error: null argp");
	while (*argp != CI_END) {
		token *t = getToken(ANYTOKEN);
		switch (t->type) {
			case EOI: throw caosException("Unexpected end of input");
			case TOK_CONST:
				{
					if (t->constval.getType() == INTEGER) {
						int val = t->constval.getInt();
						if (val >= -(1 << 24) && val < (1 << 24)) {
							emitOp(CAOS_CONSTINT, val);
							break;
						}
					}
					current->consts.push_back(t->constval);
					emitOp(CAOS_CONST, current->consts.size() - 1);
					break;
				}
			case TOK_BYTESTR:
				{
					current->bytestrs.push_back(t->bytestr);
					emitOp(CAOS_BYTESTR, current->bytestrs.size() - 1);
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
					if (t->word.size() == 4
						&&	((t->word[1] == 'v' && (t->word[0] == 'o' || t->word[0] == 'm'))
							  || (t->word[0] == 'v' && t->word[1] == 'a'))
						&&  isdigit(t->word[2]) && isdigit(t->word[3])) {
						int vidx = atoi(t->word.c_str() + 2);
						opcode_t op;
						switch(t->word[0]) {
							case 'v':
								op = CAOS_VAXX;
								break;
							case 'o':
								op = CAOS_OVXX;
								break;
							case 'm':
								op = CAOS_MVXX;
								break;
							default:
								assert(0 && "UNREACHABLE");
						}
						emitOp(op, vidx);
						break;
					}
					if (t->word.size() == 4 && strncmp(t->word.c_str(), "obv", 3) && isdigit(t->word[3])) {
						emitOp(CAOS_OVXX, atoi(t->word.c_str() + 3));
						break;
					}
					if (t->word.size() == 4 && strncmp(t->word.c_str(), "var", 3) && isdigit(t->word[3])) {
						emitOp(CAOS_VAXX, atoi(t->word.c_str() + 3));
						break;
					}
					const cmdinfo *ci = readCommand(t, std::string("expr "));
					if (ci->argc)
						readExpr(ci->argtypes);
					emitOp(CAOS_CMD, d->cmd_index(ci));
					break;
				}
			default: throw caosException("Unexpected token");
		}
		argp++;
	}
}

int caosScript::readCond() {
	token *t = getToken(TOK_WORD);
#define TWORD(lc, uc) else if (t->word == #lc) return C ## uc;
	if (0) {} // get things started
	TWORD(eq, EQ)
	TWORD(gt, GT)
	TWORD(ge, GE)
	TWORD(lt, LT)
	TWORD(le, LE)
	TWORD(ne, NE)
	TWORD(bt, BT)
	TWORD(bf, BF)
	else throw caosException(std::string("Unexpected non-condition word: ") + t->word);
#undef TWORD
}

void caosScript::parseCondition() {
	const static ci_type onearg[] = { CI_ANYVALUE, CI_END };
	emitOp(CAOS_CONSTINT, 1);

	bool nextIsAnd = true;
	while (1) {
		readExpr(onearg);
		int cond = readCond();
		readExpr(onearg);
		emitOp(CAOS_COND, cond | (nextIsAnd ? CAND : COR));

		token *peek = tokenPeek();
		if (!peek) break;
		if (peek->type != TOK_WORD) break;
		if (peek->word == "and") {
			getToken();
			nextIsAnd = true;
		} else if (peek->word == "or") {
			getToken();
			nextIsAnd = false;
		} else break;
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
			if (state != ST_INSTALLER)
				throw caosException("Unexpected SCRP");
			state = ST_BODY;
			// TODO: better validation
			int fmly = getToken(TOK_CONST)->constval.getInt();
			int gnus = getToken(TOK_CONST)->constval.getInt();
			int spcs = getToken(TOK_CONST)->constval.getInt();
			int scrp = getToken(TOK_CONST)->constval.getInt();
			scripts.push_back(shared_ptr<script>(new script(d, filename, fmly, gnus, spcs, scrp)));
			current = scripts.back();
		} else if (t->word == "rscr") {
			if (state == ST_INSTALLER || state == ST_BODY)
				state = ST_REMOVAL;
			else
				throw caosException("Unexpected RSCR");
			current = removal = shared_ptr<script>(new script(d, filename));
		} else if (t->word == "endm") {
			if (state == ST_BODY) {
				state = ST_INSTALLER;
				current = installer;
			} else {
				// I hate you. Die in a fire.
				emitOp(CAOS_DIE, -1);
				putBackToken(t);
				return;
			}
			// No we will not emit c_ENDM() thankyouverymuch
		} else if (t->word == "enum"
				|| t->word == "esee"
				|| t->word == "etch"
				|| t->word == "epas"
				|| t->word == "econ") {
			int nextreloc = current->newRelocation();
			// XXX: copypasta
			const cmdinfo *ci = readCommand(t, std::string("cmd "));
			if (ci->argc) {
				if (!ci->argtypes)
					std::cerr << "Missing argtypes for command " << t->word << "; probably unimplemented." << std::endl;
				readExpr(ci->argtypes);
			}
			emitOp(CAOS_CMD, d->cmd_index(ci));
			emitOp(CAOS_JMP, nextreloc);
			int startp    = current->getNextIndex();
			parseloop(ST_ENUM, NULL);
			current->fixRelocation(nextreloc);
			emitOp(CAOS_ENUMPOP, startp);
		} else if (t->word == "next") {
			if (state != ST_ENUM)
				throw caosException("Unexpected NEXT");
			emitOp(CAOS_CMD, d->cmd_index(d->find_command("cmd next")));
			return;
		} else if (t->word == "subr") {
			// Yes, this will work in a doif or whatever. This is UB, it may
			// be made to not compile later.
			t = getToken(TOK_WORD);
			std::string label = t->word;
			emitOp(CAOS_STOP, 0);
			current->affixLabel(label);
		} else if (t->word == "gsub") {
			t = getToken(TOK_WORD);
			std::string label = t->word;
			emitOp(CAOS_GSUB, current->getLabel(label));
		} else if (t->word == "loop") {
			int loop = current->getNextIndex();
			emitOp(CAOS_CMD, d->cmd_index(d->find_command("cmd loop")));
			parseloop(ST_LOOP, (void *)&loop);			
		} else if (t->word == "untl") {
			if (state != ST_LOOP)
				throw caosException("Unexpected UNTL");
			// TODO: zerocost logic inversion - do in c_UNTL()?
			int loop = *(int *)info;
			int out  = current->newRelocation();
			parseCondition();
			emitOp(CAOS_CMD, d->cmd_index(d->find_command("cmd untl")));
			emitOp(CAOS_CJMP, out);
			emitOp(CAOS_JMP, loop);
			current->fixRelocation(out);
			return;
		} else if (t->word == "ever") {
			if (state != ST_LOOP)
				throw caosException("Unexpected EVER");
			int loop = *(int *)info;
			emitOp(CAOS_JMP, loop);
			return;
		} else if (t->word == "reps") {
			const static ci_type types[] = { CI_NUMERIC, CI_END };
			readExpr(types);
			int loop = current->getNextIndex();
			parseloop(ST_REPS, (void *)&loop);
		} else if (t->word == "repe") {
			if (state != ST_REPS)
				throw caosException("Unexpected repe");
			emitOp(CAOS_DECJNZ, *(int *)info);
			return;
		} else if (t->word == "doif" || t->word == "elif") {
			std::string key("cmd ");
			key += t->word;
			if (t->word == "elif" && state == ST_DOIF) {
				struct doifinfo *di = (struct doifinfo *)info;
				if (di->failreloc) {
					emitOp(CAOS_JMP, di->donereloc);
					current->fixRelocation(di->failreloc);
					di->failreloc = 0;
				}
			}
			struct doifinfo info;
			info.failreloc = current->newRelocation();
			info.donereloc = current->newRelocation();
			int okreloc = current->newRelocation();

			parseCondition();
			emitOp(CAOS_CMD, d->cmd_index(d->find_command(key.c_str())));
			emitOp(CAOS_CJMP, okreloc);
			emitOp(CAOS_JMP, info.failreloc);
			current->fixRelocation(okreloc);
			parseloop(ST_DOIF, (void *)&info);
			if (info.failreloc)				
				current->fixRelocation(info.failreloc);
			current->fixRelocation(info.donereloc);
			emitOp(CAOS_CMD, d->cmd_index(d->find_command("cmd endi")));
		} else if (t->word == "else") {
			if (state != ST_DOIF)
				throw caosException("Unexpected ELSE");
			struct doifinfo *di = (struct doifinfo *)info;
			if (!di->failreloc)
				throw caosException("Duplicate ELSE");
			emitOp(CAOS_JMP, di->donereloc);
			current->fixRelocation(di->failreloc);
			di->failreloc = 0;
			emitOp(CAOS_CMD, d->cmd_index(d->find_command("cmd else")));
		} else if (t->word == "endi") {
			if (state != ST_DOIF)
				throw caosException("Unexpected ENDI");
			return;
		} else {
			const cmdinfo *ci = readCommand(t, std::string("cmd "));
			if (ci->argc) {
				if (!ci->argtypes)
					std::cerr << "Missing argtypes for command " << t->word << "; probably unimplemented." << std::endl;
				readExpr(ci->argtypes);
			}
			emitOp(CAOS_CMD, d->cmd_index(ci));
		}
	}
}
			

/* vim: set noet: */
