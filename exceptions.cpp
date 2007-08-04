/*
 *  exceptions.cpp
 *  openc2e
 *
 *  Created by Bryan Donlan on Sun 22 Jan 2006.
 *  Copyright (c) 2006 Bryan Donlan. All rights reserved.
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
#include "exceptions.h"
#include "bytecode.h"
#include "token.h"


const char *caosException::cacheDesc() const throw() {
    if (cache)
        return cache;
    if (filename) {
        std::ostringstream oss;
        oss << desc;
        oss << " at " << filename;
        if (line != -1)
            oss << ":" << line;
        if (op) {
            try {
                std::string od = dumpOp(*op);
                oss << " (op " << od << ")";
            } catch (std::exception &e) {
                oss << " (op dump failed with e.what() " << e.what() << ")";
            }
        }
        cache = strdup(oss.str().c_str());
    } else {
        cache = desc;
    }
    return cache;
}

std::string parseFailure::prettyPrint() const {
    std::ostringstream oss;
    std::string filename = this->filename;
    if (filename == "")
        filename = std::string("(UNKNOWN)");
    oss << "Parse error at line ";
    if (lineno == -1)
        oss << "(UNKNOWN)";
    else
        oss << lineno;
    oss << " in file " << filename << ": " << what();
    if (!context)
        oss << std::endl;
    else {
        oss << " near:" << std::endl;
        int toklen = -1, stlen = 0;
        for (int i = 0; i < context->size(); i++) {
            std::string tokstr = (*context)[i].format();
            if (i == ctxoffset) {
                toklen = tokstr.size();
            } else if (toklen == -1) {
                stlen += tokstr.size() + 1;
            }
            oss << tokstr << " ";
        }
        oss << std::endl;
        if (toklen != -1) {
            for (int i = 0; i < stlen; i++)
                oss << " ";
            for (int i = 0; i < toklen; i++)
                oss << "^";
            oss << std::endl;
        }
    }
    return oss.str();
}
