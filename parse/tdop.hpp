#include "token.hpp"
#include "ast.hpp"
        int precedence(TKSymbol oper) {
            switch (oper) {
                case TK_LAMBDA: return 10;
                case TK_ASSIGN: return 20;
                case TK_ASSIGN_SUM: return 21;
                case TK_ASSIGN_DIFF: return 21;
                case TK_AND: return 24;
                case TK_OR: return 24;
                case TK_LT: return 25;
                case TK_GT: return 25;
                case TK_EQU: return 25;
                case TK_NEQ: return 25;
                case TK_LTE: return 25;
                case TK_GTE: return 25;
                case TK_MATCHRE: return 25;
                case TK_SUB: return 50;
                case TK_ADD: return 50;
                case TK_MUL: return 60;
                case TK_DIV: return 60;
                case TK_MOD: return 60;
                case TK_RANDOM:
                case TK_NEW:
                case TK_INCREMENT:
                case TK_DECREMENT:
                case TK_PERIOD:
                case TK_LB:
                case TK_TRUE:
                case TK_FALSE:
                case TK_PUSH:
                case TK_APPEND:
                case TK_FIRST:
                case TK_REST:
                case TK_GET:
                case TK_POP:
                case TK_EMPTY:
                case TK_SIZE:
                case TK_LPAREN: return 100;
                default:
                    break;
            }
            return 10;
        }
        astnode* parseFirst(int prec) {
            astnode* t = nullptr;
            switch (lookahead()) {
                case TK_NUM:
                case TK_STRING:
                case TK_TRUE:
                case TK_FALSE:
                case TK_NIL: {
                    t = new astnode(CONST_EXPR, current());
                    match(lookahead());
                } break;
                case TK_RANDOM: {
                    t = new astnode(CONST_EXPR, current());
                    match(TK_RANDOM);
                    match(TK_LPAREN);
                    if (!expect(TK_RPAREN)) {
                        t->left = primary();
                    }
                    match(TK_RPAREN);
                } break;
                case TK_SUB:
                case TK_NOT: {
                    t = new astnode(UOP_EXPR, current());
                    match(lookahead());
                    t->left = parseExpr(80);
                } break;
                case TK_ID: {
                    t = new astnode(ID_EXPR, current());
                    match(lookahead());
                } break;
                case TK_NEW: {
                    t = new astnode(BLESS_EXPR, current());
                    match(TK_NEW);
                    t->left = new astnode(ID_EXPR, current());
                    match(TK_ID);
                    match(TK_LPAREN);
                    t->right = argsList();
                    match(TK_RPAREN);
                } break;
                case TK_LAMBDA: {
                    t = new astnode(LAMBDA_EXPR, current());
                    match(TK_LAMBDA);
                    match(TK_LPAREN);
                    t->left = paramList();
                    match(TK_RPAREN);
                    if (expect(TK_LCURLY)) {
                        match(TK_LCURLY);
                        t->right = stmt_list();
                        match(TK_RCURLY);
                    } else if (expect(TK_PRODUCE)) {
                        match(TK_PRODUCE);
                        t->right = expression();
                    }
                } break;
                case TK_LB: {
                    t = new astnode(LISTCON_EXPR, current());
                    match(TK_LB);
                    t->left = argsList();
                    match(TK_RB);
                } break;
                default:
                    break;
            }
            return t;
        }
        astnode* parseRest(astnode* lhs, int prec) {
            
            return lhs;
        }
        astnode* parseExpr(int prec) {
            astnode* t = parseFirst(prec);
            while (!done() && prec < precedence(lookahead())) {
                t = parseRest(t, prec);
            }
            return t;
        }