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
    public:
        Token(TKSymbol sym = TK_EOI, string st = "<nil>") : symbol(sym), strval(st), depth(-1) { }
        TKSymbol getSymbol() { return symbol; }
        string getString() { return strval; }
        int scopeLevel() { return depth; }
        void setScopeLevel(int level) { depth = level; }
};

#endif