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
    return std::string("TODO VM");
}
