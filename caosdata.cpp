/*
 *  caosdata.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Tue Jun 01 2004.
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

/*
 Automatically generated by makedocs.pl.
*/

#include "caosVM_cmdinfo.h"

void setupCommandPointers() {
	cmds = new cmdinfo[247];
	funcs = new cmdinfo[512];

	CMDDEF(RTAR, 3)
DBLCMDDEF("NEW:")
DBLCMD("NEW: SIMP", NEW_SIMP, 7)
FUNCDEF(TARG, 0)
FUNCDEF(NULL, 0)
CMDDEF(ATTR, 1)
FUNCDEF(ATTR, 0)
CMDDEF(TARG, 1)
CMDDEF(KILL, 1)
CMDDEF(NEXT, 0)
CMDDEF(SCRX, 4)
CMDDEF(BASE, 1)
FUNCDEF(FMLY, 0)
FUNCDEF(GNUS, 0)
FUNCDEF(SPCS, 0)
FUNCDEF(PLNE, 0)
FUNCDEF(PNTR, 0)
CMDDEF(META, 4)
CMDDEF(CMRT, 1)
DBLCMDDEF("DBG:")
DBLCMD("DBG: OUTS", DBG_OUTS, 1)
DBLCMD("DBG: OUTV", DBG_OUTV, 1)
FUNCDEF(GAME, 1)
CMDDEF(SCRP, 0)
CMDDEF(ENDM, 0)
DBLCMDDEF("STIM")
DBLCMD("STIM WRIT", STIM_WRIT, 3)
CMDDEF(DOIF, 0)
cmds[phash_cmd(*(int *)"DOIF")].needscondition = true;
CMDDEF(ELIF, 0)
cmds[phash_cmd(*(int *)"ELIF")].needscondition = true;
CMDDEF(ELSE, 0)
CMDDEF(ENDI, 0)
CMDDEF(REPS, 1)
CMDDEF(REPE, 0)
CMDDEF(LOOP, 0)
CMDDEF(EVER, 0)
CMDDEF(UNTL, 0)
cmds[phash_cmd(*(int *)"UNTL")].needscondition = true;
CMDDEF(GSUB, 1)
CMDDEF(SUBR, 1)
CMDDEF(RETN, 0)
FUNCDEF(ADDM, 5)
CMDDEF(BRMI, 2)
CMDDEF(MAPD, 2)
CMDDEF(MAPK, 0)
CMDDEF(INST, 0)
CMDDEF(SLOW, 0)
CMDDEF(LOCK, 0)
CMDDEF(UNLK, 0)
CMDDEF(WAIT, 1)
FUNCDEF(PACE, 0)
FUNCDEF(VAxx, 0)
CMDDEF(SETV, 2)
CMDDEF(SETA, 2)
FUNCDEF(OVxx, 0)
CMDDEF(MODV, 2)
CMDDEF(ADDV, 2)
CMDDEF(SUBV, 2)
CMDDEF(NEGV, 1)
CMDDEF(MULV, 2)
FUNCDEF(RAND, 2)
CMDDEF(RSCR, 0)

}
