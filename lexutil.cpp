#include "lexutil.h"
#include "token.h"
vector<int> bytestr;
string temp_str;

static token *peektok = NULL;
token lasttok;

void lexreset() {
    bytestr = vector<int>();
    temp_str = "";
    peektok = NULL;
}

token *getToken(toktype expected = 0) {
    token *ret = tokenPeek();
    peektok = NULL;
    return ret;
}

token *tokenPeek() {
    if (peektok)
        return peektok;
    if (!yylex())
        return NULL;
    peektok = &lasttok;
