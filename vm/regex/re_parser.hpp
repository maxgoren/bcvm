#ifndef re_parser_hpp
#define re_parser_hpp
#include <iostream>
using namespace std;
const int LITERAL = 1;
const int OPERATOR = 2;
const int CHCLASS = 3;
struct re_ast {
    int type;
    char c;
    string ccl;
    re_ast* left;
    re_ast* right;
    re_ast(string cl, int t) : type(t), c('['), ccl(cl), left(nullptr), right(nullptr) { }
    re_ast(char ch, int t) : type(t), c(ch), ccl(""), left(nullptr), right(nullptr) { }
};

void preorder(re_ast* t, int d) {
    d++;
    if (t != nullptr) {
        for (int i = 0; i < d; i++) cout<<" ";
        if (t->type == 1 || t->type == 2)
            cout<<t->c<<endl;
        else cout<<t->ccl<<endl;
        preorder(t->left, d+1);
        preorder(t->right, d+1);
    }
    d--;
}


class REParser {
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
        re_ast* factor() {
            re_ast* t;
            if (lookahead() == '(') {
                match('(');
                t = anchordexprs();
                match(')');
            } else if (isdigit(lookahead()) || isalpha(lookahead()) || lookahead() == '.') {
                t = new re_ast(lookahead(), 1);
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
                t = new re_ast(ccl, 3);
            }

            if (lookahead() == '*' || lookahead() == '+' || lookahead() == '?') {
                re_ast* n = new re_ast(lookahead(), 2);
                match(lookahead());
                n->left = t;
                t = n;
            }
            return t;
        }
        re_ast* term() {
            re_ast* t = factor();
            if (lookahead() == '(' || (lookahead() == '.' || isdigit(lookahead()) || isalpha(lookahead()) || lookahead() == '[')) {
                re_ast* n = new re_ast('@', 2);
                n->left = t;
                n->right = term();
                t = n;
            }
            return t;
        }
        re_ast* expr() {
            re_ast* t = term();
            if (lookahead() == '|') {
                re_ast* n = new re_ast('|', 2);
                match('|');
                n->left = t;
                n->right = expr();
                t = n;
            }
            return t;
        }
        re_ast* anchordexprs() {
            re_ast* t = nullptr;
            if (lookahead() == '^') {
                t = new re_ast('^', 2);
                advance();
                t->left = expr();
            } else {
                t = expr();
            }
            if (t != nullptr && lookahead() == '$') {
                advance();
                if (t->c == '^')
                    t->right = new re_ast('$', 2);
                else {
                    re_ast* n = new re_ast('$', 2);
                    n->left = t;
                    t = n;
                }
            }
            return t;
        }
    public:
        REParser() {

        }
        re_ast* parse(string pat) {
            rexpr = pat; pos = 0;
            return anchordexprs();
        }
};


#endif