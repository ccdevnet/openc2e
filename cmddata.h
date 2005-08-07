#ifndef CMDDATA_H
#define CMDDATA_H 1

extern "C" struct cmdinfo {
    const char *name;
    const char *fullname;
    const char *docs;
    int argc;
    // TODO: arg types
};

extern const cmdinfo cmds[];
void registerAutoDelegates();

#endif


