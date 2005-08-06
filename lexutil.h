#ifndef LEXUTIL_H
#define LEXUTIL_H 1

#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <string>
#include <cstdio>
#include <cctype>
#include <vector>

#include "token.h"

using namespace std;

void lexreset();

extern vector<int> bytestr;
extern string temp_str;

static inline int make_int(int v) {
    lasttok.type = TOK_CONST;
    lasttok.constval.setInt(v);
    return 1;
}

static inline int make_bin(const char *str) {
    int temp = 0, pos;
    str++; // skip %
    for (pos = 0; pos < 8; pos++) {
        temp <<= 1;
        temp += str[pos] - '0';
    }
    return make_int(temp);
//    return temp;
}

static inline int make_float(float f) {
    lasttok.type = TOK_CONST;
    lasttok.constval.setFloat(f);
    return 1;
}

static inline int make_word(const char *str) {
    int len = 0;
    string result;
    while (isalpha(*str) || isdigit(*str)
            || *str == ':' || *str == '?'
            || *str == '!'
          ) {
        result += *str++;
    }
    lasttok.type = TOK_WORD;
    transform(result.begin(), result.end(), result.begin(), (int(*)(int))tolower);
    lasttok.word = result;
    return 1;
}

static inline void push_string_escape(char c) {
    switch (c) {
        case 'n':
            temp_str += '\n';
            break;
        case 'r':
            temp_str += '\r';
            break;
        case 't':
            temp_str += '\t';
            break;
        default:
            temp_str += c;
            break;
    }
}

static inline void push_string_lit(char c) {
    temp_str += c;
}

static inline int make_string() {
    lasttok.type = TOK_CONST;
    lasttok.constval.setString(temp_str);
    temp_str = "";
    return 1;
}

static inline int push_bytestr(int bs) {
    bytestr.push_back(bs);
    return 1;
}

static inline int make_bytestr() {
    lasttok.type = TOK_BYTESTR;
    lasttok.bytestr = bytestr;
    bytestr.clear();
    return 1;
}
   
static inline void parse_error(const char *str) {
    char sbuf[32];
    strncpy(sbuf, str, 31);
    sbuf[31] = '\0';
    printf("parse error near: %s\n", sbuf);
    exit(1);
}

#endif

