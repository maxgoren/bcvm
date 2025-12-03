        int getPrec(TKSymbol symbol) {

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
            while (!done() && prec < getPrec(lookahead())) {
                t = parseRest(t, prec);
            }
            return t;
        }