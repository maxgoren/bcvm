#ifndef st_resolver_hpp
#define st_resolver_hpp
#include <iostream>
#include "../parse/ast.hpp"
#include "scopingst.hpp"
#include <unordered_map>
using namespace std;

const int GLOBAL_SCOPE = -1;
const int LOCAL_SCOPE = 0;

class STBuilder {
    private:
        ScopingST* symTable;
        string nameBlock() {
            static int bnum = 0;
            return "Block" + to_string(bnum++);
        }
        string nameLambda() {
            static int n = 0;
            return "lambdafunc" + to_string(n++);
        }
        void buildStatementST(astnode* t) {
            if (t == nullptr)
                return;
            switch (t->stmt) {
                case DEF_CLASS_STMT: {
                    symTable->openObjectScope(t->left->token.getString());
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                    symTable->closeScope();
                } break;
                case DEF_STMT: {
                    symTable->openFunctionScope(t->token.getString(), -1);
                    for (auto it = t->left; it != nullptr; it = it->next) {
                        buildSymbolTable(it);
                    }
                    buildSymbolTable(t->right);
                    symTable->closeScope();
                }  break;
                case BLOCK_STMT: {
                    t->token.setString(nameBlock());
                    symTable->openFunctionScope(t->token.getString(), -1);
                    buildSymbolTable(t->left);
                    symTable->closeScope();
                } break;
                case LET_STMT: {
                    switch (t->left->expr) {
                        case ID_EXPR: {
                            buildExpressionST(t->left, true); 
                        } break;
                        case BIN_EXPR: {
                            auto binexpr = t->left;
                            if (binexpr->right->expr == BLESS_EXPR) {
                                symTable->copyObjectScope(binexpr->left->token.getString(), binexpr->right->left->token.getString());
                            } else {
                                buildExpressionST(binexpr->left, true);
                                buildExpressionST(binexpr->right, false);
                            }
                        } break;          
                    }
                } break;
                case IF_STMT: {
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                } break;
                case ELSE_STMT: {
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                } break;
                case WHILE_STMT: {
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                } break;
                case EXPR_STMT: {
                    buildSymbolTable(t->left);
                } break;
                case RETURN_STMT: {
                    buildSymbolTable(t->left);
                } break;
                case STMT_LIST: {
                    buildStatementST(t->left);
                } break;
                default: 
                break;
            }
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
                        }
                    } else if (symTable->lookup(t->token.getString()).addr == -1) {    
                        cout<<"Error: Unknown variable name: "<<t->token.getString()<<endl;
                    } 
                } break;
                case FUNC_EXPR: {
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                } break;
                case LAMBDA_EXPR: {
                    string name = nameLambda();
                    t->token.setString(name);
                    symTable->openFunctionScope(t->token.getString(), -1);
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                    symTable->closeScope();
                } break;
                case BLESS_EXPR: {
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                } break;
                case SUBSCRIPT_EXPR: {
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                } break;
                case FIELD_EXPR: {
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->next);
                } break;
                case RANGE_EXPR: {
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->next);
                } break;
                default: 
                    buildExpressionST(t->left, fromLet);
                    buildExpressionST(t->right, fromLet);
                    break;
            }
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
        STBuilder() { }
        void buildSymbolTable(astnode* ast, ScopingST* st) {
            symTable = st;
            buildSymbolTable(ast);
        }
};

class ResolveLocals {
    private:
        int scope;
        ScopingST* st;
        vector<unordered_map<string, bool>> scopes;
        void openScope(string name) {
            SymbolTableEntry sc_info = st->lookup(name);
            if (sc_info.type == CLASSVAR) {
                st->openObjectScope(name);
            } else {
                st->openFunctionScope(name, -1);
            }
            scopes.push_back(unordered_map<string,bool>());
        }
        void closeScope() {
            scopes.pop_back();
            st->closeScope();
        }
        void declareName(string name) {
            if (scopes.empty())
                return;
            scopes.back().insert(make_pair(name, false));
        }
        void defineName(string name) {
            if (scopes.empty())
                return;
            scopes.back().at(name) = true;
        }
        void resolveName(string name, astnode* t) {
            for (int i = scopes.size() - 1; i >= 0; i--) {
                if (scopes[i].find(name) != scopes[i].end()) {
                    int depth = (scopes.size() - 1 - i);
                    t->token.setScopeLevel(depth);
                    return;
                }
            }
            t->token.setScopeLevel(GLOBAL_SCOPE);
        }
        void resolveStatement(astnode* node) {
            switch (node->stmt) {
                case IF_STMT: {
                    resolve(node->left);
                    resolve(node->right);
                } break;
                case ELSE_STMT: {
                    resolve(node->left);
                    resolve(node->right);
                } break;
                case LET_STMT: {
                    auto x = node->left;
                    while (x != nullptr) {
                        if (x->kind == EXPRNODE && x->expr == ID_EXPR)
                            break;
                        x = x->left;
                    }
                    declareName(x->token.getString());
                    resolve(node->right);
                    defineName(x->token.getString());
                    resolve(node->left);
                } break;
                case RETURN_STMT: {
                    resolve(node->left);
                } break;
                case PRINT_STMT: {
                    resolve(node->left);
                } break;
                case WHILE_STMT: {
                    resolve(node->left);
                    resolve(node->right);
                } break;
                case BLOCK_STMT: {
                    openScope(node->token.getString());
                    resolve(node->left);
                    closeScope();
                } break;
                case DEF_CLASS_STMT: {
                    declareName(node->left->token.getString());
                    defineName(node->left->token.getString());
                    openScope(node->left->token.getString());
                    resolve(node->left);
                    resolve(node->right);
                    closeScope();
                } break;
                case DEF_STMT: {
                    declareName(node->token.getString());
                    defineName(node->token.getString());
                    openScope(node->token.getString());
                    for (auto it = node->left; it != nullptr; it = it->next) {
                        declareName(it->left->token.getString());
                        defineName(it->left->token.getString());
                    }
                    resolve(node->right);
                    closeScope();
                } break;
                case EXPR_STMT: {
                    resolve(node->left);
                } break;
            }
        }
        void resolveExpression(astnode* node) {
            switch (node->expr) {
                case ID_EXPR: resolveName(node->token.getString(), node); break;
                case FUNC_EXPR: {
                    resolve(node->left);
                    resolveName(node->left->token.getString(), node->left); 
                    for (auto it = node->right; it != nullptr; it = it->next)
                        resolve(it);
                } break;
                case LAMBDA_EXPR: {
                    openScope(node->token.getString());
                    for (auto it = node->left; it != nullptr; it = it->next) {
                        resolve(it);
                    }
                    resolve(node->right);
                    closeScope();
                }break;
                case SUBSCRIPT_EXPR: {
                    resolve(node->left);
                    resolve(node->right);
                } break;
                case FIELD_EXPR: {
                    resolve(node->left);
                } break;
                case BLESS_EXPR: {
                    resolve(node->left);
                    resolve(node->right);
                } break;
                case RANGE_EXPR: {
                    resolve(node->left);
                    resolve(node->right);
                }
                default: {
                    resolve(node->left);
                    resolve(node->right);
                } break;
            }
        }
        void resolve(astnode* node) {
            if (node == nullptr)
                return;
            if (node->kind == STMTNODE) resolveStatement(node);
            else resolveExpression(node);
            resolve(node->next);
        }
    public:
        ResolveLocals() {

        }
        void resolveLocals(astnode* node, ScopingST* sym) {
            st = sym;
            resolve(node);
        }
};


#endif