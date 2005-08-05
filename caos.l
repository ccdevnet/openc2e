%{
#include <math.h>
#include <stdio.h>
#include <iostream>
//#include "token.h"
#include "lexutil.h"
%}

%option noyywrap
%x BYTESTR
%x STRING

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

"["         BEGIN(BYTESTR);

<BYTESTR>[0-9]+         push_bytestr(atoi(yytext));
<BYTESTR>[ \t\n\r]
<BYTESTR>"]"            BEGIN(INITIAL); return make_bytestr();

\"          BEGIN(STRING);
<STRING>\\.             push_string_escape(yytext[1]);
<STRING>[^"]            push_string_lit(yytext[0]);
<STRING>\"              BEGIN(INITIAL); return make_string();

[ \t\n\r]     // eat whitespace
.           parse_error(yytext);
%%
int main() {
    yyrestart(stdin);
    while(yylex());
    return 0;
}
