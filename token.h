#ifndef TOKEN_H
#define TOKEN_H 1

#include "caosVar.h"
#include <vector>
#include <iostream>

enum toktype { ANYTOKEN = 0, EOI = 0, TOK_CONST, TOK_WORD, TOK_BYTESTR };

struct token {
    int yypos; // TODO

    toktype type;
    std::vector<int> bytestr;
    std::string word;
    caosVar constval;

    token() {}
    token(const token &cp) {
        yypos = cp.yypos;
        type = cp.type;
        bytestr = cp.bytestr;
        word = cp.word;
        constval = cp.constval;
    }

    void dump_token() {
        switch(type) {
            case TOK_CONST:
                std::cout << "constval ";
                if (constval.hasInt())
                    std::cout << "int " << constval.getInt();
                else if (constval.hasFloat())
                    std::cout << "float " << constval.getFloat();
                else if (constval.hasString())
                    std::cout << "string " << constval.getString();
                else if (constval.hasAgent())
                    std::cout << "agent (BAD!)";
                else
                    std::cout << "(BAD)";
                break;
            case TOK_WORD:
                std::cout << "word " << word;
                break;
            case TOK_BYTESTR:
                {
                    std::vector<int>::iterator i = bytestr.begin();
                    std::cout << "bytestr ";
                    while (i != bytestr.end())
                        std::cout << *i++ << " ";
                }
                break;
            default:
                std::cout << "BROKEN";
                break;
        }
        std::cout << std::endl;
    }
};

token *getToken(toktype expected = ANYTOKEN);
token *tokenPeek();

extern token lasttok; // internal use only

#endif

