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

using std::string;

class unexpectedEOIexception { };

void script::addOp(caosOp *op) {
    allOps.push_back(op);
}

void script::thread(caosOp *op) {
    addOp(op);
    last->setSuccessor(op);
    last = op;
}

script::~script() {
    std::vector<class caosOp *>::iterator i = allOps.begin();
    while (i != allOps.end())
        delete *i++;
}


script::script() {
    entry = last = new caosNoop();
    allOps.push_back(entry);
}



class ScriptDialect : public Dialect {
    public:
        ScriptDialect() {
            delegates = cmd_dialect->delegates;
        }

        virtual void handleToken(class caosScript *s, token *t) {
            if (t->type == TOK_WORD && t->word == "endm") {
                stop = true;
                return;
            }
            Dialect::handleToken(s, t);
        }
};

class BaseDialect : public Dialect {
    public:
        BaseDialect() {
            delegates = cmd_dialect->delegates;
        }

        virtual void handleToken(class caosScript *s, token *t) {
            if (t->type == TOK_WORD) {
                if (t->word == "rscr") {
                    if (s->removal)
                        throw parseException("multiple rscr not allowed");
                    s->current = s->removal = new script();
                }
                if (t->word == "scrp") {
                    if (s->removal)
                        throw parseException("scrp after rscr");
                    token fmly = *getToken(TOK_CONST);
                    if (!fmly.constval.hasInt())
                       throw parseException("classifier values must be ints");
                    token gnus = *getToken(TOK_CONST);
                    if (!gnus.constval.hasInt())
                       throw parseException("classifier values must be ints");
                    token spcs = *getToken(TOK_CONST);
                    if (!spcs.constval.hasInt())
                       throw parseException("classifier values must be ints");
                    token scrp = *getToken(TOK_CONST);
                    if (!scrp.constval.hasInt())
                       throw parseException("classifier values must be ints");
                    ScriptDialect sd;
                    sd.doParse(s);
                    if (!sd.stop)
                       throw parseException("expected endm");
                }
                Dialect::handleToken(s, t);
            } // if (t->type == TOK_WORD)
        }
};

caosScript::caosScript(std::istream &in) {
    ok = 0;
    current = installer = new script();
    removal = NULL;

    yyrestart(&in);

    BaseDialect d;
    d.doParse(this);

    current->retain();
    if (removal)
        removal->retain();
    ok = 1;
}

caosScript::~caosScript() {
    if (ok) {
        installer->release();
        if (removal)
            removal->release();
    }
}
