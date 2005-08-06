#ifndef CMDDATA_H
#define CMDDATA_H 1

#include "dialect.h"

extern "C" struct cmdinfo {
    const char *name;
    const char *docs;
    int argcount;
    // TODO: arg types
    parseDelegate *parser;
};

extern const cmdinfo cmds[];
void registerAutoDelegates();

#endif


