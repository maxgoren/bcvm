#ifndef st_resolver_hpp
#define st_resolver_hpp
#include <iostream>
#include "../parse/ast.hpp"
#include "scopingst.hpp"
using namespace std;


class ScopeResolver {
    private:
        ScopingST* symTable;
        string nameBlock() {
            static int bnum = 0;
            return "Block" + to_string(bnum++);
        }
        void buildStatementST(astnode* t) {
            if (t == nullptr)
                return;
            switch (t->stmt) {
                case DEF_STMT: {
                    symTable->openFunctionScope(t->token.getString(), -1);
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                    symTable->closeScope();
                    buildSymbolTable(t->next);
                    return;
                }  break;
                case BLOCK_STMT: {
                    t->token.setString(nameBlock());
                    symTable->openFunctionScope(t->token.getString(), -1);
                    buildSymbolTable(t->left);
                    symTable->closeScope();
                    buildSymbolTable(t->next);
                    return;
                } break;
                case LET_STMT: {
                    switch (t->left->expr) {
                        case ID_EXPR: buildExpressionST(t->left, true); break;
                        case BIN_EXPR: {
                            buildExpressionST(t->left, true);
                            buildExpressionST(t->right, false);
                        } break;          
                    }
                } break;
                default: 
                break;
            }
            buildSymbolTable(t->left);
            buildSymbolTable(t->right);
            buildSymbolTable(t->next);
        } 
        void buildExpressionST(astnode* t, bool fromLet) {
            if (t == nullptr)
                return;
            switch (t->expr) {
                case ID_EXPR: {
                    if (fromLet) {
                        if (symTable->existsInScope(t->token.getString()) == false) {
                            symTable->insert(t->token.getString());
                        } else {
                            cout<<"A variable with the name "<<t->token.getString()<<" has already been declared in this scope."<<endl;
                        }
                    } else if (symTable->lookup(t->token.getString(), t->token.lineNumber()).addr == -1) {    
                        cout<<"Error: Unknown variable name: "<<t->token.getString()<<endl;
                    } 
                } break;
                case LAMBDA_EXPR: {
                    symTable->openFunctionScope(t->token.getString(), -1);
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                    symTable->closeScope();
                    buildSymbolTable(t->next);
                } break;
                default: break;
            }
            buildExpressionST(t->left, fromLet);
            buildExpressionST(t->right, fromLet);
            buildSymbolTable(t->next);
        }
        void buildSymbolTable(astnode* t) {
            if (t != nullptr) {
                switch (t->kind) {
                    case STMTNODE: {
                        buildStatementST(t);
                    } break;
                    case EXPRNODE: {
                        buildExpressionST(t, false);
                    } break;
                }
            }
        }
    public:
        ScopeResolver() { }
        void buildSymbolTable(astnode* ast, ScopingST* st) {
            symTable = st;
            buildSymbolTable(ast);
        }
};


#endif