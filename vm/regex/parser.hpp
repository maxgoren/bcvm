#ifndef re_parser_hpp
#define re_parser_hpp
#include <iostream>
using namespace std;
const int LITERAL = 1;
const int OPERATOR = 2;
const int CHCLASS = 3;
struct astnode {
    int type;
    char c;
    string ccl;
    astnode* left;
    astnode* right;
    astnode(string cl, int t) : type(t), c('['), ccl(cl), left(nullptr), right(nullptr) { }
    astnode(char ch, int t) : type(t), c(ch), ccl(""), left(nullptr), right(nullptr) { }
};

void print(astnode* t, int d) {
    d++;
    if (t != nullptr) {
        for (int i = 0; i < d; i++) cout<<" ";
        if (t->type == 1 || t->type == 3)
        cout<<t->c<<endl;
        else cout<<t->ccl<<endl;
        print(t->left, d+1);
        print(t->right, d+1);
    }
    d--;
}


class Parser {
    private:
        string rexpr;
        int pos;
        void advance() {
            if (pos < rexpr.length())
                pos++;
        }
        bool match(char c) {
            if (c == rexpr[pos]) {
                advance();
                return true;
            }
            return false;
        }
        char lookahead() {
            return rexpr[pos];
        }
        astnode* factor() {
            astnode* t;
            if (lookahead() == '(') {
                match('(');
                t = anchordexprs();
                match(')');
            } else if (isdigit(lookahead()) || isalpha(lookahead()) || lookahead() == '.') {
                t = new astnode(lookahead(), 1);
                advance();
            } else if (lookahead() == '[') {
                advance();
                string ccl;
                while (pos+1 < rexpr.length() && lookahead() != ']') {
                    ccl.push_back(lookahead());
                    advance();
                }
                if (lookahead() != ']') {
                    cout<<"Error: Unclosed character class."<<endl;
                    return nullptr;
                } else {
                    advance();
                }
                t = new astnode(ccl, 3);
            }

            if (lookahead() == '*' || lookahead() == '+' || lookahead() == '?') {
                astnode* n = new astnode(lookahead(), 2);
                match(lookahead());
                n->left = t;
                t = n;
            }
            return t;
        }
        astnode* term() {
            astnode* t = factor();
            if (lookahead() == '(' || (lookahead() == '.' || isdigit(lookahead()) || isalpha(lookahead()) || lookahead() == '[')) {
                astnode* n = new astnode('@', 2);
                n->left = t;
                n->right = term();
                t = n;
            }
            return t;
        }
        astnode* expr() {
            astnode* t = term();
            if (lookahead() == '|') {
                astnode* n = new astnode('|', 2);
                match('|');
                n->left = t;
                n->right = expr();
                t = n;
            }
            return t;
        }
        astnode* anchordexprs() {
            astnode* t = nullptr;
            if (lookahead() == '^') {
                t = new astnode('^', 2);
                advance();
                t->left = expr();
            } else {
                t = expr();
            }
            if (t != nullptr && lookahead() == '$') {
                advance();
                if (t->c == '^')
                    t->right = new astnode('$', 2);
                else {
                    astnode* n = new astnode('$', 2);
                    n->left = t;
                    t = n;
                }
            }
            return t;
        }
    public:
        Parser() {

        }
        astnode* parse(string pat) {
            rexpr = pat; pos = 0;
            return anchordexprs();
        }
};


#endif