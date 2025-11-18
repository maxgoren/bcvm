#ifndef bytecodecompiler_hpp
#define bytecodecompiler_hpp
#include "parse/ast.hpp"
#include "stackitem.hpp"
#include <map>
#include <vector>
#include <iostream>
using namespace std;

struct Scope;

struct STItem {
    string name;
    int type;
    int addr;
    int depth;
    Function* func;
    STItem(string n, int adr, int sip, Scope* s, int d) : type(2), name(n), depth(d), addr(adr), func(new Function(n, sip, s)) { }
    STItem(string n, int adr, int d) : type(1), name(n), addr(adr), depth(d), func(nullptr) { }
    STItem() : type(0), func(nullptr) { }
};

struct Scope {
    Scope* enclosing;
    map<string, STItem> symTable;
};


class ScopingST {
    private:
        Scope* st;
        int nextAddr() {
            return st->symTable.size()+1; //locals[0] is resrved for return value.
        }
        int depth(Scope* s) {
            if (s->enclosing == nullptr)
                return -1;
            auto x = s;
            int d = 0;
            while (x != nullptr) {
                d++;
                x = x->enclosing;
            }
            return d;
        }
    public:
        ScopingST() {
            st = new Scope();
            st->enclosing = nullptr;
        }
        void openFunctionScope(string name, int L1) {
            if (st->symTable.find(name) != st->symTable.end()) {
                Scope* ns = st->symTable[name].func->scope;
                ns->enclosing = st;
                st->symTable[name].func->start_ip = L1;
                st = ns;
            } else {
                Scope*  ns = new Scope;
                ns->enclosing = st;
                st->symTable[name] = STItem(name, nextAddr(), L1, ns, depth(ns));
                st = ns;
            }
        }
        void copyScope(string dest, string src) {
            if (st->symTable.find(src) != st->symTable.end()) {
                STItem item = st->symTable[src];
                if (item.type == 2) {
                    Function* ns = item.func;
                    st->symTable[dest] = STItem(dest, nextAddr(), ns->start_ip, ns->scope, depth(ns->scope));
                }
            } 
        }
        void closeScope() {
            if (st != nullptr) {
                st = st->enclosing;
            }
        }
        void insert(string name) {
            st->symTable[name] = STItem(name, nextAddr(), depth(st));
        }
        STItem lookup(string name) {
            Scope* x = st;
            while (x != nullptr) {
                if (x->symTable.find(name) != x->symTable.end())
                    return x->symTable[name];
                x = x->enclosing;
            }
            return STItem("not found", -1, -1);
        }
        int depth() {
            return depth(st);
        }
        void printScope(Scope* s, int d) {
            auto x = s;
            for (auto m : x->symTable) {
                for (int i = 0; i < d; i++) cout<<"  ";
                cout<<m.first<<": "<<m.second.addr<<", "<<m.second.depth<<endl;
                if (m.second.type == 2) {
                    printScope(m.second.func->scope,d + 1);
                }
            }
        }
        void print() {
            printScope(st, 1);
        }
};

class  ByteCodeCompiler {
    private:
        vector<Instruction> code;
        int cpos;
        int highCI;
        ScopingST symTable;
        string nextLabel() {
            static int next = 0;
            return "L" + to_string(next++);
        }
        string nameLambda() {
            static int n = 0;
            return "lambdafunc" + to_string(n++);
        }
        int scopeLevel() {
            return symTable.depth();
        }
        STItem lookup(string varname) {
            if (symTable.lookup(varname).addr == -1)
                symTable.insert(varname);
            return symTable.lookup(varname);
        }
        void emit(Instruction inst) {
            code[cpos++] = inst;
            if (cpos > highCI)
                highCI = cpos;
        }
        int skipEmit(int numSpaces) {
            cpos += numSpaces;
            return cpos;
        }
        void skipTo(int loc) {
            cpos = loc;
        }
        void restore() {
            cpos = highCI;
        }
        void emitBinaryOperator(astnode* n) {
            genCode(n->left,  n->token.getSymbol() == TK_ASSIGN);
            genCode(n->right, false);
            switch (n->token.getSymbol()) {
                case TK_ADD: emit(Instruction(binop, VM_ADD)); break;
                case TK_SUB: emit(Instruction(binop, VM_SUB)); break;
                case TK_MUL: emit(Instruction(binop, VM_MUL)); break;
                case TK_DIV: emit(Instruction(binop, VM_DIV)); break;
                case TK_LT:  emit(Instruction(binop, VM_LT)); break;
                case TK_GT:  emit(Instruction(binop, VM_GT)); break;
                case TK_EQU: emit(Instruction(binop, VM_EQU)); break;
                case TK_NEQ: emit(Instruction(binop, VM_NEQ)); break;
                case TK_ASSIGN: {
                    emit(Instruction(stglobal, scopeLevel())); break;
                } break;
            }
        }
        void emitUnaryOperator(astnode* n) {
            genCode(n->left, false);
            emit(Instruction(unop, VM_NEG));
        }
        void emitLoad(astnode* n, bool needLvalue) {
            STItem item = lookup(n->token.getString());
            if (needLvalue) {
                if (symTable.depth() == -1) {
                    emit(Instruction(ldglobaladdr, item.addr, item.depth));
                } else {
                    emit(Instruction(ldlocaladdr, item.addr, item.depth));
                }
            } else {
                if (symTable.depth() == -1) {
                    emit(Instruction(ldglobal, item.addr, item.depth));
                } else {
                    emit(Instruction(ldlocal, item.addr, item.depth));
                }
            }
        }
        void emitReturn(astnode* n) {
            genCode(n->left, false);
            emit(Instruction(retfun));
        }
        void emitPrint(astnode* n) {
            genCode(n->left, false); 
            emit(Instruction(print));
        }
        void emitConstant(astnode* n) {
            switch (n->token.getSymbol()) {
                case TK_NUM:    emit(Instruction(ldconst, stod(n->token.getString()))); break;
                case TK_STRING: emit(Instruction(ldconst, n->token.getString())); break;
                case TK_TRUE:   emit(Instruction(ldconst, true)); break;
                case TK_FALSE:  emit(Instruction(ldconst, false)); break;
                default: break;
            } 
        }
        void emitLambda(astnode* n) {
            int numArgs = 0;
            for (astnode* x = n; x != nullptr; x = x->next)
                numArgs++;
            int L1 = skipEmit(0);
            skipEmit(1);
            string name = nameLambda();
            emit(Instruction(defun, name, numArgs, 0));
            symTable.openFunctionScope(name, L1+1);
            genCode(n->right, false);
            emit(Instruction(retfun));
            int cpos = skipEmit(0);
            symTable.closeScope();
            emit(Instruction(label, nextLabel()));
            skipTo(L1);
            emit(Instruction(jump, cpos));
            restore();
            STItem fn_info = symTable.lookup(name);
            emit(Instruction(ldconst, new Closure(fn_info.func)));
            if (symTable.depth() == -1) {
                emit(Instruction(ldglobaladdr, fn_info.addr, fn_info.depth));
                emit(Instruction(stglobal, fn_info.addr));
            } else {
                emit(Instruction(ldlocaladdr, fn_info.addr, fn_info.depth));
                emit(Instruction(stlocal, fn_info.addr));
            }          
        }
        void emitFunctionCall(astnode* n) {
            STItem fn_info = functionInfo(n->left->token.getString());
            int argsCount = 0;
            for (auto x = n->right; x != nullptr; x = x->next)
                argsCount++;
            emit(Instruction(ldconst, new Closure(new Function(fn_info.name, fn_info.func->start_ip, fn_info.func->scope))));    
            emit(Instruction(call, fn_info.func->start_ip, argsCount, fn_info.addr));
            genCode(n->right, false);
            emit(Instruction(entfun, fn_info.func->start_ip));
        }
        void emitFuncDef(astnode* n) {
            int numArgs = 0;
            for (astnode* x = n->left; x != nullptr; x = x->next)
                numArgs++;
            int L1 = skipEmit(0);
            skipEmit(1);
            string name = n->token.getString();
            emit(Instruction(defun, name, numArgs, 0));
            symTable.openFunctionScope(name, L1+1);
            genCode(n->right, false);
            emit(Instruction(retfun));
            int cpos = skipEmit(0);
            symTable.closeScope();
            emit(Instruction(label, nextLabel()));
            skipTo(L1);
            emit(Instruction(jump, cpos));
            restore();
            STItem fn_info = symTable.lookup(name);
            emit(Instruction(ldconst, new Closure(fn_info.func)));
            if (symTable.depth() == -1) {
                emit(Instruction(ldglobaladdr, fn_info.addr, fn_info.depth));
                emit(Instruction(stglobal, fn_info.addr));
            } else {
                emit(Instruction(ldlocaladdr, fn_info.addr, fn_info.depth));
                emit(Instruction(stlocal, fn_info.addr));
            }            
        }
        void defineIfStmt(astnode* n) {
            if (n->right->token.getSymbol() == TK_ELSE) {
                astnode* lrc = n->right;
                genCode(n->left, false);
                int L1 = skipEmit(0);
                skipEmit(1);
                genCode(lrc->left, false);
                int L2 = skipEmit(0);
                skipEmit(1);
                genCode(lrc->right, false);
                int L3 = skipEmit(0);
                skipTo(L1);
                emit(Instruction(brf, L2+1));
                skipTo(L2);
                emit(Instruction(jump, L3));
                restore();
            } else {
                genCode(n->left, false);
                int L1 = skipEmit(0);
                skipEmit(1);
                genCode(n->right, false);
                int L2 = skipEmit(0);
                skipTo(L1);
                emit(Instruction(brf, L2));
                restore();
            }
        }
        STItem functionInfo(string name) {
            STItem item = symTable.lookup(name);
            /*if (item.type == 2 && item.func != nullptr) {
                cout<<item.func->name<<": "<<item.addr<<", "<<item.func->start_ip<<endl;
            }*/
            return item;
        }
        void genStatement(astnode* n, bool needLvalue) {
            switch (n->stmt) {
                case DEF_STMT:    { emitFuncDef(n); } break;
                case IF_STMT:     { defineIfStmt(n);   } break;
                case LET_STMT:    { genCode(n->left, false);  } break;
                case PRINT_STMT:  { emitPrint(n);      } break;
                case RETURN_STMT: { emitReturn(n);     } break;
                case EXPR_STMT: { 
                    genCode(n->left, false);
                }break;
                default: break;
            };
        }
        void genExpression(astnode* n, bool needLvalue) {
            switch (n->expr) {
                case CONST_EXPR:  { emitConstant(n);  } break;
                case ID_EXPR:     { emitLoad(n, needLvalue); } break;
                case BIN_EXPR:    { emitBinaryOperator(n); } break;
                case UOP_EXPR:    { emitUnaryOperator(n);  } break;
                case LAMBDA_EXPR: { emitLambda(n);        } break;
                case FUNC_EXPR:   { emitFunctionCall(n);   } break;
                default:
                    break;
            }
        }
        void genCode(astnode* n, bool needLvalue) {
            if (n != nullptr) {
                if (n->kind == STMTNODE) {
                    genStatement(n, needLvalue);
                } else {
                    genExpression(n, needLvalue);
                }
                genCode(n->next, false);
            }
        }
        void buildSymbolTable(astnode* t) {
            if (t == nullptr)
                return;
            switch (t->kind) {
                case STMTNODE: {
                    switch (t->stmt) {
                        case DEF_STMT: {
                            symTable.openFunctionScope(t->token.getString(), -1);
                            buildSymbolTable(t->left);
                            buildSymbolTable(t->right);
                            symTable.closeScope();
                            return;
                        } break;
                    }
                } break;
                case EXPRNODE: {
                    switch (t->expr) {
                        case ID_EXPR: lookup(t->token.getString()); break;
                        case BIN_EXPR: break;
                    }
                } break;
            }
            buildSymbolTable(t->left);
            buildSymbolTable(t->right);
        }
    public:
        ByteCodeCompiler() {
            code = vector<Instruction>(255, Instruction(halt, 0));
            cpos = 0;
            highCI = 0;
        }

        vector<Instruction> compile(astnode* n) {
            buildSymbolTable(n);
            preorder(n, 1);
            genCode(n, false);
            symTable.print();
            cout<<"Compiled Bytecode: "<<endl;
            int addr = 0;
            for (auto m : code) {
                cout<<addr<<": [0x0"<<m.op<<"("<<instrStr[m.op]<<"), "<<(m.operand[0].toString() == "(nil)" ? to_string(m.operand[0].intval):m.operand[0].toString())<<"]"<<endl;
                if (m.op == halt)
                    break;
                addr++;
            }
            cout<<"----------------"<<endl;
            return code;
        }
};

#endif