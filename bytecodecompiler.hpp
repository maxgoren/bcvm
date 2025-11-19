#ifndef bytecodecompiler_hpp
#define bytecodecompiler_hpp
#include "parse/ast.hpp"
#include "scopingst.hpp"
using namespace std;

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
            if (n->token.getSymbol() == TK_ASSIGN) {
                genCode(n->right, false);
                genCode(n->left, true);
                emit(Instruction(symTable.depth() == -1 ? stglobal:stlocal, symTable.depth()));
            } else {
                genCode(n->left,  false);
                genCode(n->right, false);
                switch (n->token.getSymbol()) {
                    case TK_ADD: emit(Instruction(binop, VM_ADD)); break;
                    case TK_SUB: emit(Instruction(binop, VM_SUB)); break;
                    case TK_MUL: emit(Instruction(binop, VM_MUL)); break;
                    case TK_DIV: emit(Instruction(binop, VM_DIV)); break;
                    case TK_MOD: emit(Instruction(binop, VM_MOD)); break;
                    case TK_LT:  emit(Instruction(binop, VM_LT)); break;
                    case TK_GT:  emit(Instruction(binop, VM_GT)); break;
                    case TK_EQU: emit(Instruction(binop, VM_EQU)); break;
                    case TK_NEQ: emit(Instruction(binop, VM_NEQ)); break;
                }
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
        void emitSubscript(astnode* n, bool isLvalue) {
            genCode(n->left, false);
            genCode(n->right, false);
            emit(Instruction(isLvalue ? stfield:ldfield));
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
                case TK_NIL:    emit(Instruction(ldconst));
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
            //emit(Instruction(ldconst, new Closure(new Function(fn_info.name, fn_info.func->start_ip, fn_info.func->scope))));    
            genExpression(n->left, false);
            if (fn_info.type == 2) {
            emit(Instruction(call, fn_info.func->start_ip, argsCount, fn_info.addr));
            } else {
            emit(Instruction(call, 0, argsCount, 0));
            }
            genCode(n->right, false);
            if (fn_info.type == 2) {
                emit(Instruction(entfun, fn_info.func->start_ip));
            } else {
                emit(Instruction(entfun, 0));
            }
        }
        void emitListConstructor(astnode* n) {
            emit(Instruction(ldconst, new vector<StackItem>()));
            for (astnode* it = n->left; it != nullptr; it = it->next) {
                genExpression(it, false);
                emit(Instruction(append));
            }
        }
        void emitBlock(astnode* n) {
            int L1 = skipEmit(0);
            emit(Instruction(call, L1+1));
            emit(Instruction(entfun, L1+2));
            genCode(n->left, false);
            emit(Instruction(retfun));
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
        void emitWhile(astnode* n) {
            int P1 = skipEmit(0);
            genCode(n->left, false);
            int L1 = skipEmit(0);
            skipEmit(1);
            genCode(n->right, false);
            emit(Instruction(jump, P1));
            int L2 = skipEmit(0);
            skipTo(L1);
            emit(Instruction(brf, L2));
            restore();
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
                case BLOCK_STMT:  { emitBlock(n);   } break;
                case IF_STMT:     { defineIfStmt(n);   } break;
                case LET_STMT:    { genCode(n->left, false);  } break;
                case PRINT_STMT:  { emitPrint(n);      } break;
                case RETURN_STMT: { emitReturn(n);     } break;
                case WHILE_STMT:  { emitWhile(n);      } break;
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
                case LISTCON_EXPR: { emitListConstructor(n); } break;
                case SUBSCRIPT_EXPR: { emitSubscript(n, needLvalue); } break;
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
                        case BLOCK_STMT: {
                            symTable.openFunctionScope(t->token.getString(), -1);
                            buildSymbolTable(t->left);
                            symTable.closeScope();
                        }
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