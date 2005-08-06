%{
#include <math.h>
#include <stdio.h>
#include <iostream>
//#include "token.h"
#include "lexutil.h"
%}

%option noyywrap
%x L_BYTESTR
%x L_STRING

STRINGBIT   \\.|[^\\]
DIGIT       [0-9]
WORDINIT    [a-zA-Z:?!]
%%

"*".*\n     // eat comments
\\.         return make_int(yytext[1]);
%[01][01][01][01][01][01][01][01]   return make_bin(yytext);
{WORDINIT}({WORDINIT}|{DIGIT})*         return make_word(yytext);
[-+]?{DIGIT}*\.{DIGIT}+  return make_float(atof(yytext));
[-+]?{DIGIT}+            return make_int(atoi(yytext));

"<"                       return make_word("lt");
">"                       return make_word("gt");
"<="                      return make_word("le");
">="                      return make_word("ge");
"<>"                      return make_word("ne");
"="                       return make_word("eq");
"&&"                      return make_word("and");
"||"                      return make_word("or");

"["         BEGIN(L_BYTESTR);

<L_BYTESTR>[0-9]+         push_bytestr(atoi(yytext));
<L_BYTESTR>[ \t\n\r]
<L_BYTESTR>"]"            BEGIN(INITIAL); return make_bytestr();

\"          BEGIN(L_STRING);
<L_STRING>\\.             push_string_escape(yytext[1]);
<L_STRING>\"              BEGIN(INITIAL); return make_string();
<L_STRING>.               push_string_lit(yytext[0]);

[ \t\n\r]     // eat whitespace
.           parse_error(yytext);
%%
int main() {
    yyrestart(stdin);
    while(yylex());
    return 0;
}
