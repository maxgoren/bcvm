#ifndef ast_hpp
#define ast_hpp
#include "token.hpp"
#include <iostream>
#include <set>
using namespace std;


enum NodeType {
    EXPRNODE, STMTNODE
};

enum ExprType {
    CONST_EXPR, ID_EXPR, BIN_EXPR, UOP_EXPR, FUNC_EXPR, LAMBDA_EXPR,
    LISTCON_EXPR, SUBSCRIPT_EXPR, LIST_EXPR, BLESS_EXPR, FIELD_EXPR,
    RANGE_EXPR
};

string exprTypeStr[] = {
    "CONST_EXPR", "ID_EXPR", "BIN_EXPR", "UOP_EXPR", "FUNC_EXPR", "LAMBDA_EXPR",
    "LISTCON_EXPR", "SUBSCRIPT_EXPR", "LIST_EXPR", "BLESS_EXPR", "FIELD_EXPR",
    "RANGE_EXPR"
};


enum StmtType {
    PRINT_STMT, WHILE_STMT, IF_STMT, ELSE_STMT, STMT_LIST, EXPR_STMT,
    LET_STMT, RETURN_STMT, DEF_STMT, DEF_CLASS_STMT, BLOCK_STMT
};

string stmtTypeStr[] = { 
    "PRINT_STMT", "WHILE_STMT", "IF_STMT", "ELSE_STMT", "STMT_LIST", "EXPR_STMT",
    "LET_STMT", "RETURN_STMT", "DEF_STMT", "DEF_CLASS_STMT", "BLOCK_STMT"
    
};

struct astnode {
    NodeType kind;
    union {
        ExprType expr;
        StmtType stmt;
    };
    Token token;
    astnode* left;
    astnode* right;
    astnode* next;
    astnode(ExprType et, Token tk) : expr(et), kind(EXPRNODE), token(tk), left(nullptr), right(nullptr), next(nullptr) { }
    astnode(StmtType st, Token tk) : stmt(st), kind(STMTNODE), token(tk), left(nullptr), right(nullptr), next(nullptr) { }
    astnode() : token(Token(TK_EOI, "fin")), left(nullptr), right(nullptr), next(nullptr) { }
};

void preorder(astnode* node, int d, set<astnode*>& seen) {
    if (node != nullptr) {
        if (seen.find(node) == seen.end()) {
            seen.insert(node);
            for (int i = 0; i < d; i++) cout<<" ";
            cout<<"[";
            switch (node->kind) {
                case EXPRNODE: cout<<exprTypeStr[node->expr]; break;
                case STMTNODE: cout<<stmtTypeStr[node->stmt]; break;
            }
            cout<<"] "<<node->token.getString()<<endl;
            preorder(node->left, d + 1, seen);
            preorder(node->right, d + 1, seen);
            preorder(node->next, d, seen);
        } else {
            cout<<"Cycle found in AST: "<<node->token.getString()<<endl;
        }
    }
}

#endif