#ifndef bytecodecompiler_hpp
#define bytecodecompiler_hpp
#include "../parse/ast.hpp"
#include "../vm/instruction.hpp"
#include "scopingst.hpp"
using namespace std;


//{ fn ok() { println "hi"; }; ok(); }


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
        SymbolTableEntry& lookup(string varname) {
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
            SymbolTableEntry item = lookup(n->token.getString());
            if (needLvalue) {
                if (item.depth == -1) {
                    emit(Instruction(ldglobaladdr, item.addr, item.depth));
                } else {
                    emit(Instruction(ldlocaladdr, item.addr, item.depth));
                }
            } else {
                if (item.depth == -1) {
                    emit(Instruction(ldglobal, item.addr, item.depth));
                } else {
                    emit(Instruction(ldlocal, item.addr, item.depth));
                }
            }
        }
        void emitListAccess(astnode* n, bool isLvalue) {
            genCode(n->left, false);
            genCode(n->right, false);
            emit(Instruction(isLvalue ? stfield:ldfield));
        }
        void emitFieldAccess(astnode* n, bool isLvalue) {
            genCode(n->left, false);
            genCode(n->right, false);
        }
        void emitSubscript(astnode* n, bool isLvalue) {
            if (n->token.getSymbol() == TK_PERIOD) {
                emitFieldAccess(n, isLvalue);
            } else {
                emitListAccess(n, isLvalue);
            }
        }
        void emitListOperation(astnode* n) {
            switch (n->token.getSymbol()) {
                case TK_APPEND: {
                    genExpression(n->left, false);
                    emit(Instruction(list_append));
                } break;
                case TK_PUSH: {
                    genExpression(n->left, false);
                    emit(Instruction(list_push));
                } break;
                case TK_SIZE: {
                    emit(Instruction(list_len));
                } break;
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
            n->token.setString(name);
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
            emitStoreFuncInEnvironment(name);
        }
        void emitFunctionCall(astnode* n) {
            SymbolTableEntry fn_info = functionInfo(n->left->token.getString());
            int argsCount = 0;
            for (auto x = n->right; x != nullptr; x = x->next)
                argsCount++;
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
            emit(Instruction(ldconst, new deque<StackItem>()));
            for (astnode* it = n->left; it != nullptr; it = it->next) {
                genExpression(it, false);
                emit(Instruction(list_append));
            }
        }
        void emitBlock(astnode* n) {
            int L1 = skipEmit(0);
            emit(Instruction(entblk, L1+2));
            genCode(n->left, false);
            emit(Instruction(retblk));
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
            emitStoreFuncInEnvironment(name);            
        }
        void emitStoreFuncInEnvironment(string name) {
            SymbolTableEntry fn_info = symTable.lookup(name);
            emit(Instruction(ldconst, new Closure(fn_info.func)));
            if (fn_info.depth == -1) {
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
        SymbolTableEntry functionInfo(string name) {
            SymbolTableEntry item = symTable.lookup(name);
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
                case EXPR_STMT: { genCode(n->left, false); } break;
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
                case LIST_EXPR: { emitListOperation(n); } break;
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
        string nameBlock() {
            static int bnum = 0;
            return "Block" + to_string(bnum++);
        }
        void buildStatementST(astnode* t) {
            switch (t->stmt) {
                case DEF_STMT: {
                    symTable.openFunctionScope(t->token.getString(), -1);
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                    symTable.closeScope();
                    buildSymbolTable(t->next);
                    return;
                }  break;
                case BLOCK_STMT: {
                    t->token.setString(nameBlock());
                    symTable.openFunctionScope(t->token.getString(), -1);
                    buildSymbolTable(t->left);
                    symTable.closeScope();
                    buildSymbolTable(t->next);
                    return;
                } break;
                case LET_STMT: {
                    astnode* x = t->left;
                    while (x != nullptr) {
                        if (x->expr == ID_EXPR && !symTable.existsInScope(x->token.getString())) {
                            symTable.insert(x->token.getString());
                            break;
                        }
                        x = x->left;
                    }
                    buildSymbolTable(t->left);
                } break;
                default: 
                break;
            }
            buildSymbolTable(t->left);
            buildSymbolTable(t->right);
        } 
        void buildExpressionST(astnode* t) {
            switch (t->expr) {
                case ID_EXPR: {
                    if (symTable.lookup(t->token.getString()).addr == -1) {
                        cout<<"Error: Unknown variable name: "<<t->token.getString()<<endl;
                    } 
                } break;
                default: break;
            }
            buildSymbolTable(t->left);
            buildSymbolTable(t->right);
        }
        void buildSymbolTable(astnode* t) {
            if (t != nullptr) {
                switch (t->kind) {
                    case STMTNODE: {
                        buildStatementST(t);
                    } break;
                    case EXPRNODE: {
                        buildExpressionST(t);
                    } break;
                }
                buildSymbolTable(t->next);
            }
        }
    public:
        ByteCodeCompiler() {
            code = vector<Instruction>(255, Instruction(halt, 0));
            cpos = 0;
            highCI = 0;
        }

        vector<Instruction> compile(astnode* n) {
            cout<<"Build Symbol Table: "<<endl;
            buildSymbolTable(n);
            cout<<"Compiling..."<<endl;
            genCode(n, false);
            cout<<"Compiled Bytecode: "<<endl;
            int addr = 0;
            for (auto m : code) {
                cout<<addr<<": [0x0"<<m.op<<"("<<instrStr[m.op]<<"), "<<(m.operand[0].toString() == "(nil)" ? to_string(m.operand[0].intval):m.operand[0].toString())<<"]"<<endl;
                if (m.op == halt)
                    break;
                addr++;
            }
            cout<<"Symbol table: ";
            symTable.print();
            cout<<"----------------"<<endl;
            return code;
        }
};

#endif