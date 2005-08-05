#ifndef LEXUTIL_H
#define LEXUTIL_H 1

#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <string>
#include <cstdio>
#include <cctype>
#include <vector>

using namespace std;

static vector<int> bytestr;
static string temp_str;

static int make_int(int v) {
    printf("matched int: %d\n", v);
    return 1;
}

static int make_bin(const char *str) {
    int temp = 0, pos;
    str++; // skip %
    for (pos = 0; pos < 8; pos++) {
        temp <<= 1;
        temp += str[pos] - '0';
    }
    printf("matched bin: '%d'\n", temp);
    return 1;
//    return temp;
}

static int make_float(float f) {
    printf("matched float: %f\n", f);
    return 1;
}

static int make_word(const char *str) {
    int len = 0;
    string result;
    const char *start = str;
    char *buf = NULL;
    while (isalpha(*str) || isdigit(*str)
            || *str == ':' || *str == '?'
            || *str == '!'
          ) {
        result += *str++;
    }
    printf("matched word: '%s'\n", result.c_str());
    return 1;
}

static void push_string_escape(char c) {
    temp_str += c; // XXX
}

static void push_string_lit(char c) {
    temp_str += c;
}

static int make_string() {
    cout << "made string: " << temp_str << endl;
    temp_str = "";
    return 1;
}

static int push_bytestr(int bs) {
    bytestr.push_back(bs);
    return 1;
}

static int make_bytestr() {
    vector<int>::iterator i = bytestr.begin();
    cout << "matched bytestr: [ ";
    while (i != bytestr.end()) {
        cout << *i++ << " ";
    }
    cout << "]" << endl;
    return 1;
}
   
static void parse_error(const char *str) {
    char sbuf[32];
    strncpy(sbuf, str, 31);
    sbuf[31] = '\0';
    printf("parse error near: %s\n", sbuf);
    exit(1);
}

#endif

