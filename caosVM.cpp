/*
 *  caosVM.cpp
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
#include "openc2e.h"
#include "World.h"
#include "bytecode.h"
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

class caosAbort : public caosOp {
    public:
        void execute(caosVM *vm) {
            cerr << "Bad! Some opcode forgot to set its successor" << endl;
            caos_assert(false);
        }
} abortop;

caosVM::caosVM(const AgentRef &o)
    : vm(this)
{
    owner = o;
    currentscript = NULL;
    cip = nip = NULL;
	resetCore();
}

inline void caosVM::runOp() {
    cip = nip;
    nip = &abortop; // detect misbehaved ops
    if (cip == NULL) {
        stop();
        return;
    }
    result.reset(); // xxx this belongs in opcode maybe
    cip->execute(this);
    if (!result.isNull())
        valueStack.push_back(result);
}

inline void caosVM::stop() {
    cip = nip = NULL;
    if (currentscript)
        currentscript->release();
    currentscript = NULL;
    resetCore();
}

void caosVM::runEntirely(script *s) {
	currentscript = s;
    currentscript->retain();
    cip = nip = s->entry;
	while (nip)
		runOp();
    stop(); // probably redundant, but eh
}

bool caosVM::fireScript(script &s, bool nointerrupt) {
	if (lock) return false; // can't interrupt scripts which called LOCK
	if (currentscript && nointerrupt) return false; // don't interrupt scripts with a timer script

	resetScriptState();
	currentscript = &s;
    currentscript->retain();
    cip = nip = s.entry;
	return true;
}

void caosVM::resetScriptState() {
    stop();
    resetCore();
}

void caosVM::resetCore() {

    valueStack.clear();
    miscStack.clear(); // XXX memory leak?

    inst = lock = 0;
    timeslice = 0;

    outputstream = &cout;

    _it_ = NULL;
    targ = owner;

	_p_[0].reset(); _p_[0].setInt(0); _p_[1].reset(); _p_[1].setInt(0);
	for (unsigned int i = 0; i < 100; i++) { var[i].reset(); var[i].setInt(0); }
}

void caosVM::tick() {
    runTimeslice(5);
}

void caosVM::runTimeslice(int units) {
    timeslice = units;
    while (currentscript && (timeslice || inst))
        runOp();
}

