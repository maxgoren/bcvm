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
            return symbol == lookahead();
        }
        bool match(TKSymbol symbol) {
            if (expect(symbol)) {
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
            astnode d, *t = &d;
            while (!expect(TK_RPAREN)) {
                if (expect(TK_COMMA))
                    match(TK_COMMA);
                t->next = statement();
                t = t->next;
            }
            return d.next;
        }
        astnode* parseFunctionCallAndSubscripts(astnode* n) {
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
                    astnode* ma = new astnode(FIELD_EXPR, current());
                    match(TK_PERIOD);
                    ma->left = n;
                    ma->right = primary();
                    if (ma->right->expr == LIST_EXPR)
                    ma->expr = LIST_EXPR;
                    n = ma;
                }
            }
            return n;
        }
        astnode* primary() {
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
            } else if (expect(TK_TRUE)) {
                n = new astnode(CONST_EXPR, current());
                match(TK_TRUE);
            } else if (expect(TK_FALSE)) {
                n = new astnode(CONST_EXPR, current());
                match(TK_FALSE);
            } else if (expect(TK_NIL)) {
                n = new astnode(CONST_EXPR, current());
                match(TK_NIL);
            } else if (expect(TK_LPAREN)) {
                match(TK_LPAREN);
                n = expression();
                match(TK_RPAREN);
            } else if (expect(TK_LAMBDA)) {
                n = new astnode(LAMBDA_EXPR, current());
                match(TK_LAMBDA);
                n = functionBody(n);
            } else if (expect(TK_LB)) {
                n = new astnode(LISTCON_EXPR, current());
                match(TK_LB);
                n->left = argsList();
                match(TK_RB);
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
            } else if (expect(TK_EMPTY)) {
                n = new astnode(LIST_EXPR, current());
                match(TK_EMPTY);
                match(TK_LPAREN);
                match(TK_RPAREN);
            } else if (expect(TK_NEW)) {
                n = new astnode(BLESS_EXPR, current());
                match(TK_NEW);
                n->left = new astnode(ID_EXPR, current());
                match(TK_ID);
                match(TK_LPAREN);
                n->right = argsList();
                match(TK_RPAREN);
            } else if (expect(TK_RANDOM)) {
                n = new astnode(CONST_EXPR, current());
                match(TK_RANDOM);
                match(TK_LPAREN);
                if (!expect(TK_RPAREN)) {
                    n->left = primary();
                }
                match(TK_RPAREN);
            }
            if (n != nullptr && (n->expr == ID_EXPR || n->expr == LIST_EXPR)) {
                n = parseFunctionCallAndSubscripts(n);
            }
            return n;
        }
        astnode* unary() {
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
            if (expect(TK_RANGE)) {
                astnode* t = new astnode(RANGE_EXPR, current());
                match(TK_RANGE);
                t->left = n;
                t->right = primary();
                n = t;
            }
            return n;
        }
        astnode* factor() {
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
            astnode* n = relopExpr();
            while (expect(TK_EQU) || expect(TK_NEQ) || expect(TK_MATCHRE)) {
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
        astnode* assignExpr() {
            astnode* n = logicalExpr();
            while (expect(TK_ASSIGN) || expect(TK_ASSIGN_DIFF) || expect(TK_ASSIGN_SUM)) {
                astnode* q = new astnode(BIN_EXPR, current());
                match(lookahead());
                q->left = n;
                q->right = logicalExpr();
                n = q;
            }
            return n;
        }
        astnode* expression() {
            astnode* n = assignExpr();
            while (expect(TK_COLON)) {
                astnode* q = new astnode(BIN_EXPR, current());
                match(lookahead());
                q->left = n;
                q->right = assignExpr();
                n = q;
            }
            return n;
        }
        astnode* functionBody(astnode* n) {
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
            return n;
        }
        astnode* parseIfStmt() {
            astnode* n = new astnode(IF_STMT, current());
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
            return n;
        }
        astnode* parseWhileStmt() {
            astnode* n = new astnode(WHILE_STMT, current());
            match(TK_WHILE);
            match(TK_LPAREN);
            n->left = expression();
            match(TK_RPAREN);
            match(TK_LCURLY);
            n->right = stmt_list();
            match(TK_RCURLY);
            return n;
        }
        astnode* parseVarDec() {
            astnode* n = new astnode(LET_STMT, current());
            match(TK_LET);
            astnode* t = new astnode(ID_EXPR, current());
            match(TK_ID);
            if (expect(TK_COLON)) {
                match(TK_COLON);
                n->right = new astnode(ID_EXPR, current());
                match (TK_ID);
            }
            if (expect(TK_ASSIGN)) {
                astnode* r = new astnode(BIN_EXPR, current());
                match(TK_ASSIGN);
                r->left = t;
                r->right = expression();
                t = r;
            }
            n->left = t;
            return n;
        }
        astnode* parseSequence() {
            match(TK_LCURLY);
            astnode* t = stmt_list();
            match(TK_RCURLY);
            return t;
        }
        astnode* parseBlock() {
            astnode* n = new astnode(BLOCK_STMT, current());
            n->left = parseSequence();
            return n;
        }
        astnode* parseFuncDef() {
            astnode* n = new astnode(DEF_STMT, current());
            match(TK_FN);
            n->token = current();
            match(TK_ID);
            n = functionBody(n);
            return n;
        }
        astnode* parseClassDef() {
            astnode* n = new astnode(DEF_CLASS_STMT, current());
            match(TK_CLASS);
            n->left = expression();
            n->right = parseSequence();
            return n;
        }
        astnode* parsePrintStmt() {
            astnode* n = new astnode(PRINT_STMT, current());
            match(lookahead());
            n->left = expression();
            return n;
        }
        astnode* parseReturn() {
            astnode* n = new astnode(RETURN_STMT, current());
            match(TK_RETURN);
            n->left = expression();
            return n;
        }
        astnode* statement() {
            astnode* n = nullptr;
            switch (lookahead()) {
                case TK_PRINT:
                case TK_PRINTLN:  n = parsePrintStmt(); break;
                case TK_CLASS:    n = parseClassDef();break;
                case TK_IF:       n = parseIfStmt(); break;
                case TK_WHILE:    n = parseWhileStmt();break;
                case TK_FN:       n = parseFuncDef(); break;
                case TK_LET:      n = parseVarDec(); break;
                case TK_LCURLY:   n = parseBlock(); break;
                case TK_RETURN:   n = parseReturn(); break;
                default:
                    n = new astnode(EXPR_STMT, current());
                    n->left = expression();
            }
            if (expect(TK_SEMI))
                match(TK_SEMI);
            return n;
        }
        astnode* stmt_list() {
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
            preorder(p, 1);
            return p;
        }
}; 

#endif