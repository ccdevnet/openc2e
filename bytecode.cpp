/*
 *  bytecode.cpp
 *  openc2e
 *
 *  Created by Bryan Donlan on Wed 07 Dec 2005.
 *  Copyright (c) 2005 Bryan Donlan. All rights reserved.
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
#include <boost/format.hpp>

using boost::format;
using boost::str;

const char *cnams[] = {
    NULL,
    "EQ",
    "LT",
    "LE",
    "GT",
    "GE",
    "NE",
    NULL
};

std::string dumpOp(caosOp op) {
    int arg = op.argument; // weird C++ issues
    switch (op.opcode) {
        case CAOS_NOP:
            return std::string("NOP");
        case CAOS_DIE:
            return str(format("DIE %d") % arg);
        case CAOS_STOP:
            return std::string("STOP");
        case CAOS_CMD:
            return str(format("CMD %d") % arg);
        case CAOS_COND:
            return str(format("COND %s %s") % (arg & CAND ? "AND" : "OR") % cnams[arg & CMASK]);
        case CAOS_CONST:
            return str(format("CONST %d") % arg);
        case CAOS_CONSTINT:
            return str(format("CONSTINT %d") % arg);
        case CAOS_VAXX:
            return str(format("VA%02d") % arg);
        case CAOS_OVXX:
            return str(format("OV%02d") % arg);
        case CAOS_MVXX:
            return str(format("MV%02d") % arg);
        case CAOS_CJMP:
            return str(format("CJMP %08d") % arg);
        case CAOS_JMP:
            return str(format("JMP %08d") % arg);
        case CAOS_DECJNZ:
            return str(format("DECJNZ %08d") % arg);
        case CAOS_GSUB:
            return str(format("GSUB %08d") % arg);
        case CAOS_ENUMPOP:
            return str(format("ENUMPOP %08d") % arg);
        default:
            return str(format("UNKNOWN %02x %06x") % arg);
    }
};
