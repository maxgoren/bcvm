#ifndef token_hpp
#define token_hpp
#include <iostream>
#include "lexer_matrix.h"
using namespace std;

class Token {
    private:
        TKSymbol symbol;
        string strval;
        int depth;
        int lineNum;
    public:
        Token(TKSymbol sym = TK_EOI, string st = "<nil>", int ln = 0) : symbol(sym), strval(st), depth(-1), lineNum(ln) { }
        TKSymbol getSymbol() { return symbol; }
        string getString() { return strval; }
        void setString(string s) { strval = s; }
        int scopeLevel() { return depth; }
        void setScopeLevel(int level) { depth = level; }
        void setLineNum(int ln) { lineNum = ln; }
        int lineNumber() { return lineNum; }
};

#endif