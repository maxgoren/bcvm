#ifndef parser_hpp
#define parser_hpp
#include <iostream>
#include <vector>
#include "ast.hpp"
#include "token.hpp"
using namespace std;


class Parser {
    private:    
        vector<Token> tokens;
        int tpos;
        void init(vector<Token>& tk) {
            tokens = tk;
            tpos = 0;
        }
        void advance() {
            tpos++;
        }
        bool done() {
            return lookahead() == TK_EOI;
        }
        bool expect(TKSymbol symbol) {
            //cout<<"Lookahead: "<<current().getSymbol()<<endl;
            //cout<<"Expecting: "<<symbol<<endl;
            return symbol == lookahead();
        }
        bool match(TKSymbol symbol) {
            if (expect(symbol)) {
                //cout<<"\n----------------\nMatched "<<current().getString()<<"\n-------------------"<<endl;
                advance();
                return true;
            }
            cout<<"Mismatched token: "<<current().getString()<<"Thought it was "<<symbol<<endl;
            return false;
        }
        Token& current() {
            return tokens[tpos];
        }
        TKSymbol lookahead() {
            return tokens[tpos].getSymbol();
        }
        astnode* argsList() {
            //cout<<"args list"<<endl;
            astnode d, *t = &d;
            while (!expect(TK_RPAREN) && !expect(TK_RB)) {
                if (expect(TK_COMMA))
                    match(TK_COMMA);
                t->next = expression();
                t = t->next;
            }
            return d.next;
        }
        astnode* paramList() {
            //cout<<"param list"<<endl;
            astnode d, *t = &d;
            while (!expect(TK_RPAREN)) {
                if (expect(TK_COMMA))
                    match(TK_COMMA);
                t->next = statement();
                t = t->next;
            }
            return d.next;
        }
        astnode* primary() {
            //cout<<"primary expr"<<endl;
            astnode* n = nullptr;
            if (expect(TK_NUM)) {
                n = new astnode(CONST_EXPR, current());
                match(TK_NUM);
            } else if (expect(TK_STRING)) {
                n = new astnode(CONST_EXPR, current());
                match(TK_STRING);
            } else if (expect(TK_ID)) {
                n = new astnode(ID_EXPR, current());
                match(TK_ID);
            } else if (expect(TK_LPAREN)) {
                match(TK_LPAREN);
                n = expression();
                match(TK_RPAREN);
            } else if (expect(TK_TRUE)) {
                n = new astnode(CONST_EXPR, current());
                match(TK_TRUE);
            } else if (expect(TK_FALSE)) {
                n = new astnode(CONST_EXPR, current());
                match(TK_FALSE);
            } else if (expect(TK_LAMBDA)) {
                n = new astnode(LAMBDA_EXPR, current());
                match(TK_LAMBDA);
                match(TK_LPAREN);
                n->left = paramList();
                match(TK_RPAREN);
                if (expect(TK_LCURLY)) {
                    match(TK_LCURLY);
                    n->right = stmt_list();
                    match(TK_RCURLY);
                } else if (expect(TK_PRODUCE)) {
                    match(TK_PRODUCE);
                    n->right = expression();
                }
            } else if (expect(TK_LB)) {
                n = new astnode(LISTCON_EXPR, current());
                match(TK_LB);
                n->left = argsList();
                match(TK_RB);
            } else if (expect(TK_SIZE)) {
                n = new astnode(LIST_EXPR, current());
                match(TK_SIZE);
                n->left = expression();
            } else if (expect(TK_APPEND)) {
                n = new astnode(LIST_EXPR, current());
                match(TK_APPEND);
                match(TK_LPAREN);               
                n->left = expression();
                match(TK_RPAREN);
            } else if (expect(TK_PUSH)) {
                n = new astnode(LIST_EXPR, current());
                match(TK_PUSH);
                match(TK_LPAREN);
                n->left = expression();
                match(TK_RPAREN);
            } else if (expect(TK_SIZE)) {
                n = new astnode(LIST_EXPR, current());
                match(TK_SIZE);
                match(TK_LPAREN);
                match(TK_RPAREN);
            }
            if (n != nullptr && (n->expr == ID_EXPR || n->expr == LIST_EXPR)) {
                while (expect(TK_LPAREN) || expect(TK_LB) || expect(TK_PERIOD)) {
                    if (expect(TK_LPAREN)) {
                        astnode* fc = new astnode(FUNC_EXPR, current());
                        fc->left = n;
                        match(TK_LPAREN);
                        fc->right = argsList();
                        match(TK_RPAREN);
                        n = fc;
                    } else if (expect(TK_LB)) {
                        astnode* ss = new astnode(SUBSCRIPT_EXPR, current());
                        match(TK_LB);
                        ss->left = n;
                        ss->right = expression();
                        match(TK_RB);
                        n = ss;
                    } else if (expect(TK_PERIOD)) {
                        astnode* ma = new astnode(SUBSCRIPT_EXPR, current());
                        match(TK_PERIOD);
                        ma->left = n;
                        ma->right = expression();
                        n = ma;
                    }
                }
            }
            return n;
        }
        astnode* unary() {
            //cout<<"unary expr"<<endl;
            astnode* n = nullptr;
            if (expect(TK_SUB) || expect(TK_NOT)) {
                n = new astnode(UOP_EXPR, current());
                match(lookahead());
                n->left = unary();
            } else {
                n = primary();
            }
            if (expect(TK_INCREMENT) || expect(TK_DECREMENT)) {
                astnode* t = new astnode(UOP_EXPR, current());
                match(lookahead());
                t->left = n;
                n = t;
            }
            return n;
        }
        astnode* factor() {
            //cout<<"factor"<<endl;
            astnode* n = unary();
            while (expect(TK_MUL) || expect(TK_DIV) || expect(TK_MOD)) {
                astnode* q = new astnode(BIN_EXPR, current());
                match(lookahead());
                q->left = n;
                q->right = unary();
                n = q;
            }
            return n;
        }
        astnode* term() {
           // cout<<"term"<<endl;
            astnode* n = factor();
            while (expect(TK_ADD) || expect(TK_SUB)) {
                astnode* q = new astnode(BIN_EXPR, current());
                match(lookahead());
                q->left = n;
                q->right = factor();
                n = q;
            }
            return n;
        }
        astnode* relopExpr() {
         //   cout<<"relop expr"<<endl;
            astnode* n = term();
            while (expect(TK_LT) || expect(TK_GT) || expect(TK_LTE) || expect(TK_GTE)) {
                astnode* q = new astnode(BIN_EXPR, current());
                match(lookahead());
                q->left = n;
                q->right = term();
                n = q;
            }
            return n;
        }
        astnode* compExpr() {
         //   cout<<"comp expr"<<endl; 
            astnode* n = relopExpr();
            while (expect(TK_EQU) || expect(TK_NEQ)) {
                astnode* q = new astnode(BIN_EXPR, current());
                match(lookahead());
                q->left = n;
                q->right = relopExpr();
                n = q;
            }
            return n;
        }
        astnode* logicalExpr() {
            astnode* n = compExpr();
            while (expect(TK_LOGIC_AND) || expect(TK_LOGIC_OR)) {
                astnode* q = new astnode(BIN_EXPR, current());
                match(lookahead());
                q->left = n;
                q->right = compExpr();
                n = q;
            }
            return n;
        }
        astnode* expression() {
       //     cout<<"expr"<<endl;
            astnode* n = logicalExpr();
            while (expect(TK_ASSIGN)) {
                astnode* q = new astnode(BIN_EXPR, current());
                match(TK_ASSIGN);
                q->left = n;
                q->right = logicalExpr();
                n = q;
            }
            return n;
        }
        astnode* statement() {
         //   cout<<"statement"<<endl;
            astnode* n = nullptr;
            switch (lookahead()) {
                case TK_PRINTLN: {
                    n = new astnode(PRINT_STMT, current());
                    match(TK_PRINTLN);
                    n->left = expression();
                } break;
                case TK_IF: {
                    n = new astnode(IF_STMT, current());
                    match(TK_IF);
                    match(TK_LPAREN);
                    n->left = expression();
                    match(TK_RPAREN);
                    match(TK_LCURLY);
                    n->right = stmt_list();
                    match(TK_RCURLY);
                    if (expect(TK_ELSE)) {
                        astnode* e = new astnode(ELSE_STMT, current());
                        match(TK_ELSE);
                        match(TK_LCURLY);
                        e->right = stmt_list();
                        e->left = n->right;
                        n->right = e;
                        match(TK_RCURLY);
                    }
                } break;
                case TK_WHILE: {
                    n = new astnode(WHILE_STMT, current());
                    match(TK_WHILE);
                    match(TK_LPAREN);
                    n->left = expression();
                    match(TK_RPAREN);
                    match(TK_LCURLY);
                    n->right = stmt_list();
                    match(TK_RCURLY);
                } break;
                case TK_FN: {
                    n = new astnode(DEF_STMT, current());
                    match(TK_FN);
                    n->token = current();
                    match(TK_ID);
                    match(TK_LPAREN);
                    if (!expect(TK_RPAREN))
                        n->left = paramList();
                    match(TK_RPAREN);
                    match(TK_LCURLY);
                    n->right = stmt_list();
                    match(TK_RCURLY);
                } break;
                case TK_LCURLY: {
                    n = new astnode(BLOCK_STMT, current());
                    match(TK_LCURLY);
                    n->left = stmt_list();
                    match(TK_RCURLY);
                } break;
                case TK_LET: {
                    n = new astnode(LET_STMT, current());
                    match(TK_LET);
                    n->left = expression();
                } break;
                case TK_RETURN: {
                    n = new astnode(RETURN_STMT, current());
                    match(TK_RETURN);
                    n->left = expression();
                } break;
                default:
                    n = new astnode(EXPR_STMT, current());
                    n->left = expression();
            }
            if (expect(TK_SEMI))
                match(TK_SEMI);
            return n;
        }
        astnode* stmt_list() {
         //   cout<<"stmt list"<<endl;
            astnode* x = statement();
            astnode* m = x;
            while (!expect(TK_EOI) && !expect(TK_RCURLY)) {
                astnode* q = statement();
                if (m == nullptr) {
                    m = x = q;
                } else {
                    m->next = q;
                    m = q;
                }
            }
            return x;
        }
    public:
        Parser() {

        }
        astnode* parse(vector<Token> tokens) {
            init(tokens);
            astnode* p = stmt_list();
            cout<<"Parse complete."<<endl;
            set<astnode*> seen;
            preorder(p, 1, seen);
            return p;
        }
}; 

#endif