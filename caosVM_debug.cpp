/*
 *  caosVM_debug.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Sun Oct 24 2004.
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
#include "Agent.h"
#include "World.h"
#include <iostream>
#include "cmddata.h"
#include <cctype>
#include "dialect.h"
#include <algorithm>

// #include "malloc.h" <- unportable horror!
#include <sstream>
#include <boost/format.hpp>

using std::cerr;
using std::cout;

/**
 DBG: OUTS (command) val (string)
 %status maybe

 Outputs a string to the debug log.
*/
void caosVM::c_DBG_OUTS() {
	VM_PARAM_STRING(val)
	
	cout << val << std::endl;
}

/**
 DBGM (command) val (bareword)
 %status maybe
 %pragma variants c1 c2
 %pragma implementation caosVM::c_DBG_OUTS
*/

/**
 DBG: OUTV (command) val (decimal)
 %status maybe
 
 Outputs a decimal value to the debug log.
*/
void caosVM::c_DBG_OUTV() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_VALUE(val)

	if (val.hasFloat()) {
		cout << boost::format("%0.06f") % val.getFloat();
	} else if (val.hasInt()) {
		cout << val.getInt();
	} else if (val.hasVector()) {
		const Vector<float> &v = val.getVector();
		*outputstream << boost::format("(%0.6f, %0.6f)") % v.x % v.y;
	} else throw badParamException();

	cout << std::endl;
}

/**
 DBGV (command) val (integer)
 %status maybe
 %pragma variants c2
 %pragma implementation caosVM::c_DBG_OUTV
*/

/**
 DBUG (command) val (integer)
 %status maybe
 %pragma variants c1 c2
*/
void caosVM::c_DBUG() {
	inst = true;
	c_DBG_OUTV();
}

/**
 UNID (integer)
 %status maybe

 Returns the unique ID of the target agent.
 This is currently no good for persisting.

XXX: when serialization support works, this might well become good for
	 persisting :)
*/
void caosVM::v_UNID() {
	VM_VERIFY_SIZE(0)
	valid_agent(targ);
	result.setInt(targ->getUNID());
}

/**
 AGNT (agent) id (integer)
 %status maybe

 Returns the agent with the given UNID, or NULL if agent has been deleted.
*/
void caosVM::v_AGNT() {
	VM_VERIFY_SIZE(1)
	VM_PARAM_INTEGER(id)

	result.setAgent(world.lookupUNID(id));
}

/**
 DBG: MALLOC (command)
 %status stub

 Dumps some random memory stats to stderr.
*/
void caosVM::c_DBG_MALLOC() {
	VM_VERIFY_SIZE(0)
	
	// more unportable horror!
/*	struct mallinfo mi = mallinfo();
#define MPRINT(name) \
	fprintf(stderr, "%10s = %d\n", #name, mi. name)
	MPRINT(arena);
	MPRINT(ordblks);
	MPRINT(smblks);
	MPRINT(hblks);
	MPRINT(hblkhd);
	MPRINT(usmblks);
	MPRINT(fsmblks);
	MPRINT(uordblks);
	MPRINT(fordblks);
	MPRINT(keepcost);
	malloc_stats(); */
	
	/*std::cerr << "caosSlab free=" << caosVarSlab.free_elements() <<
				 " used=" << caosVarSlab.used_elements() <<
				 " total=" << caosVarSlab.total_elements() <<
				 std::endl;*/
}
	
/**
 DBG: DUMP (command)
 %status ok
 %pragma variants c1 c2 c3 ca cv

 Dumps the current script's bytecode to stderr.
*/
void caosVM::c_DBG_DUMP() {
	std::cerr << vm->currentscript->dump();
}	

/**
 DBG: TRACE (command) enable (integer)
 %status ok
 %pragma variants c1 c2 c3 ca cv

 Enables/disables opcode tracing to cerr.
*/
void caosVM::c_DBG_TRACE() {
	VM_PARAM_INTEGER(en)

	vm->trace = en;
}

/**
 MANN (command) cmd (string)
 %status stub
 
 Looks up documentation on the given command and spits it on the current output socket.
*/
void caosVM::c_MANN() {
	VM_PARAM_STRING(cmd)
	// TODO VM
	result.setString("bug bd about this function.");
}

/**
 DBG: DISA (command) family (integer) genus (integer) species (integer) event (integer)
 %status ok

 Dumps the "bytecode" of the indicated script to the current output channel.
 Note that this isn't really bytecode yet (though that's a possible future
 improvement).

 If the script is not found no output will be generated.
 */
void caosVM::c_DBG_DISA() {
	VM_PARAM_INTEGER(event)
	VM_PARAM_INTEGER(species)
	VM_PARAM_INTEGER(genus)
	VM_PARAM_INTEGER(family)

	shared_ptr<script> s = world.scriptorium.getScript(family, genus, species, event);
	if (!s) return;

	*outputstream << s->dump();
}

/**
 DBG: ASRT (command) condition (condition)
 %pragma parser new AssertParser()
 %status maybe

 Blows up unless the given condition is true.
*/
void caosVM::c_DBG_ASRT() {
	throw caosException("DBG: ASRT condition failed");
}

/**
 DBG: IDNT (string) agent (agent)
 %status ok

 (openc2e-only)
 Return a nicely-formatted string identifying the classifier of the agent,
 using the catalogue to find the name if possible.
*/
void caosVM::v_DBG_IDNT() {
	VM_PARAM_AGENT(a)
	
	if (!a)
		result.setString("(null)");
	else
		result.setString(a->identify());
}

/**
 DBG: PROF (command)
 %status stub

 Dumps the current agent profiling information to the output stream, in CSV format.
*/
void caosVM::c_DBG_PROF() {
	// TODO
}

/**
 DBG: CPRO (command)
 %status stub

 Clears the current agent profiling information.
*/
void caosVM::c_DBG_CPRO() {
	// TODO
}

/**
 DBG: STOK (string) bareword (bareword)
 %status ok

 Returns the bare token in 'bareword' as a string.
*/
void caosVM::v_DBG_STOK() {
	VM_PARAM_STRING(bareword)
	
	result.setString(bareword);
}

/* vim: set noet: */
