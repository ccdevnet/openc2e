#include "lexutil.h"
#include "lex.yy.h"
#include "token.h"
vector<int> bytestr;
string temp_str;

static token *peektok = NULL;
token lasttok;

static yyFlexLexer lexer; // XXX!

void yyrestart(std::istream *is) {
    lexreset();
    lexer.yyrestart(is);
}

void lexreset() {
    bytestr = vector<int>();
    temp_str = "";
    peektok = NULL;
}

token *getToken(toktype expected) {
    token *ret = tokenPeek();
    peektok = NULL;
    if (expected != ANYTOKEN && ret->type != expected)
        throw parseException("unexpected token");
    return ret;
}

token *tokenPeek() {
    if (peektok)
        return peektok;
    if (!lexer.yylex())
        return NULL;
    peektok = &lasttok;
    return peektok;
}
