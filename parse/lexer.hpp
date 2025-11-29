#ifndef lexer_hpp
#define lexer_hpp
#include <iostream>
#include <vector>
#include "token.hpp"
#include "lexer_matrix.h"
#include "../buffer.hpp"
using namespace std;

class Lexer {
    private:
        CharBuffer* buffer;
        bool shouldSkip(char ch);
        Token makeLexToken(TKSymbol symbol, char* text, int length);
        Token nextToken();
    public:
        Lexer();
        vector<Token> lex(CharBuffer* buffer);
};

Lexer::Lexer() { }

Token Lexer::makeLexToken(TKSymbol symbol, char* text, int length) {
    return Token(symbol, string(text, length));
}

Token Lexer::nextToken() {
    int state = 1;
    int last_match = 0;
    int match_len = 0;
    int len = 0;
    bool inquote = false;
    int start = buffer->markStart();
    for (char p = buffer->get(); !buffer->done(); buffer->advance(), len++) {
        state = matrix[state][buffer->get()];
        if (state > 0 && accept[state] > -1) {
            last_match = state;
            match_len = len;
        }

        if (buffer->get() == '"') {
            if (!inquote) inquote = true;
            else {
                inquote = false;
                buffer->advance();
                break;
            }
        }
        if (state < 1) {
            break;
        }
    }
    if (last_match == 0) {
        return {TK_EOI};
    }
    return Token((TKSymbol)accept[last_match], buffer->sliceFromStart(match_len), buffer->lineNo());
}

bool Lexer::shouldSkip(char c) {
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n'); 
}

vector<Token> Lexer::lex(CharBuffer* buff) {
    buffer = buff;
    vector<Token> tokens;
    for (; !buffer->done();) { 
        while (shouldSkip(buffer->get())) buffer->advance();
        Token next;
        next = nextToken();
        if (next.getSymbol() != TK_EOI) {
            tokens.push_back(next);
        } else {
            buffer->advance();
        }
    }
    tokens.push_back(Token(TK_EOI, "<fin>"));
    return tokens;
}
#endif