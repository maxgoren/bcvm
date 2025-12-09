#ifndef bytecodecompiler_hpp
#define bytecodecompiler_hpp
#include <iomanip>
#include <unordered_map>
#include "../parse/ast.hpp"
#include "../vm/instruction.hpp"
#include "../vm/constpool.hpp"
#include "scopingst.hpp"
#include "stresolver.hpp"
using namespace std;


class  ByteCodeGenerator {
    private:
        bool noisey;
        vector<Instruction> code;
        int cpos;
        int highCI;
        ScopingST symTable;
        STBuilder sr;
        ResolveLocals rl;
        int scopeLevel() {
            return symTable.depth();
        }
        SymbolTableEntry& lookup(string varname, int ln) {
            return symTable.lookup(varname, ln);
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
                if (noisey) {
                    cout<<"Compiling Assignment: "<<endl;
                    cout<<"Step one: stack value (from right)"<<endl;
                }
                genCode(n->right, false);
                if (n->left->token.getSymbol() == TK_LB) {
                    if (noisey) cout<<"Step two: handle list access."<<endl;
                    emitListAccess(n->left, true);
                } else if (n->left->token.getSymbol() == TK_PERIOD) {
                    emitFieldAccess(n->left, true);
                } else {
                    if (noisey) cout<<"Step two: stack address of var name (from left)"<<endl;
                    emitLoad(n->left, true);
                    if (noisey) cout<<"Step three: emit appropriate store instructioin"<<endl;
                    emitStore(n);
                }
            } else {
                if (noisey) cout<<"Compiling BinOp: "<<n->token.getString()<<endl;
                genCode(n->left,  false);
                genCode(n->right, false);
                switch (n->token.getSymbol()) {
                    case TK_ADD:  emit(Instruction(binop, VM_ADD)); break;
                    case TK_SUB:  emit(Instruction(binop, VM_SUB)); break;
                    case TK_MUL:  emit(Instruction(binop, VM_MUL)); break;
                    case TK_DIV:  emit(Instruction(binop, VM_DIV)); break;
                    case TK_MOD:  emit(Instruction(binop, VM_MOD)); break;
                    case TK_LT:   emit(Instruction(binop, VM_LT)); break;
                    case TK_GT:   emit(Instruction(binop, VM_GT)); break;
                    case TK_LTE:  emit(Instruction(binop, VM_LTE)); break;
                    case TK_GTE:  emit(Instruction(binop, VM_GTE)); break;
                    case TK_EQU:  emit(Instruction(binop, VM_EQU)); break;
                    case TK_NEQ:  emit(Instruction(binop, VM_NEQ)); break;
                    case TK_LOGIC_AND:  emit(Instruction(binop, VM_LOGIC_AND)); break;
                    case TK_LOGIC_OR:   emit(Instruction(binop, VM_LOGIC_OR)); break;
                }
            }
        }
        void emitUnaryOperator(astnode* n) {
            genCode(n->left, false);
            switch (n->token.getSymbol()) {
                case TK_INCREMENT: { 
                    emit(Instruction(incr)); 
                    genCode(n->left, true);
                } break;
                case TK_DECREMENT: { 
                    emit(Instruction(decr)); 
                    genCode(n->left, true);
                } break;
                default:
                    emit(Instruction(unop, VM_NEG));
            }
        }
        void emitLoadAddress(SymbolTableEntry& item, astnode* n) {
            emit(Instruction(ldaddr, item.addr));
            if (noisey) cout << "LDLOCALADDR: " << n->token.getString()<<"scopelevel="<<n->token.scopeLevel() << " depth=" << item.depth<< endl;
        }
        void emitLoad(astnode* n, bool needLvalue) {
            if (noisey) cout<<"Compiling ID expression: ";
            SymbolTableEntry item = lookup(n->token.getString(), n->token.lineNumber());
            int depth = n->token.scopeLevel();
            if (needLvalue) {
                emitLoadAddress(item, n);
            } else {
                if (depth == GLOBAL_SCOPE) {
                    emit(Instruction(ldglobal, item.addr));
                    if (noisey) cout << "LDGLOBAL: " << n->token.getString()<<"scopelevel="<<n->token.scopeLevel() << " depth=" << item.depth<< endl;
                } else if (depth == 0) {
                    emit(Instruction(ldlocal, item.addr));
                    if (noisey) cout << "LDLOCAL: " << n->token.getString()<<", scopelevel= "<<n->token.scopeLevel() << " depth= " <<item.depth<< endl;
                } else {
                    emit(Instruction(ldupval, item.addr, depth));
                    if (noisey) cout<< "LDUPVAL: " << n->token.getString()<<", scopelevel= "<<n->token.scopeLevel() << " depth= " <<item.depth<< endl;
                }
            }
        }
        void emitStore(astnode* n) {
            SymbolTableEntry item = symTable.lookup(n->left->token.getString(), 1);
            int depth = n->left->token.scopeLevel();
            if (depth == GLOBAL_SCOPE) {
                emit(Instruction(stglobal, item.addr));
                if (noisey) cout << "STGLOBAL: " << n->left->token.getString()<<", scopelevel= "<<n->left->token.scopeLevel() << " depth= " <<item.depth << endl;
            } else if (depth == 0) {
                emit(Instruction(stlocal, item.addr));
                if (noisey) cout << "STLOCAL: " << n->left->token.getString()<<", scopelevel= "<<n->left->token.scopeLevel() << " depth= " << item.depth<< endl;
            } else {
                emit(Instruction(stupval, depth));
                if (noisey) cout << "STUPVAL: " << n->left->token.getString()<<", scopelevel= "<<n->left->token.scopeLevel() << " depth= " << item.depth<< endl;
            }

        }
        void emitListAccess(astnode* n, bool isLvalue) {
            genExpression(n->left, false);
            genExpression(n->right, false);
            emit(Instruction(isLvalue ? stidx:ldidx));
        }
        void emitFieldAccess(astnode* n, bool isLvalue) {
            cout<<"Emitting Field access for "<<n->left->token.getString()<<"."<<n->right->token.getString()<<endl;
            emitLoad(n->left, false);
            int fieldname = symTable.getConstPool().insert(gc.alloc(new string(n->right->token.getString())));
            emit(Instruction(isLvalue ? stfield:ldfield, fieldname));
        }
        void emitListOperation(astnode* listExpr) {
            auto listname = listExpr->left;
            auto operation = listExpr->right;
            if (operation == nullptr)
                return;
            switch (operation->token.getSymbol()) {
                case TK_APPEND: {
                    genExpression(listname, false);
                    genExpression(operation->left, false);
                    emit(Instruction(list_append));
                } break;
                case TK_PUSH: {
                    genExpression(listname, false);
                    genExpression(operation->left, false);
                    emit(Instruction(list_push));
                } break;
                case TK_SIZE: {
                    genExpression(listname, false);
                    emit(Instruction(list_len));
                } break;
            }
        }
        void emitReturn(astnode* n) {
            genCode(n->left, false);
            emit(Instruction(retfun));
        }
        void emitPrint(astnode* n) {
            if (noisey) cout<<"Compiling Print Statement: "<<endl;
            genExpression(n->left, false); 
            emit(Instruction(print));
            if (n->token.getSymbol() == TK_PRINTLN)
                emit(Instruction(newline));
        }
        void emitConstant(astnode* n) {
            if (noisey) cout<<"Compiling Constant: "<<n->token.getString()<<endl;
            switch (n->token.getSymbol()) {
                case TK_NUM:    {
                    int idx = symTable.getConstPool().insert(StackItem(stod(n->token.getString())));
                    emit(Instruction(ldconst, idx));  
                } break;
                case TK_STRING: {
                    int idx = symTable.getConstPool().insert(StackItem(n->token.getString()));
                    emit(Instruction(ldconst, idx));  
                } break;
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
            string name = n->token.getString();
            emit(Instruction(defun, name, numArgs, 0));
            symTable.openFunctionScope(name, L1+1);
            genCode(n->right, false);
            emit(Instruction(retfun));
            int cpos = skipEmit(0);
            symTable.closeScope();
            skipTo(L1);
            emit(Instruction(jump, cpos));
            restore();
            emitStoreFuncInEnvironment(n, true);
        }
        void emitBlessExpr(astnode* n) {
            cout<<"Let their be life..."<<endl;
            int L1 = skipEmit(0);
            skipEmit(1);
            int i = 0;
            for (auto x = n->right; x != nullptr; x = x->next) {
                genCode(x, false);
                emit(Instruction(stfield, ++i));
            }
            skipTo(L1);
            string name = n->left->token.getString();
            int cpIdx = symTable.lookupClass(name) == nullptr ? -1:symTable.lookupClass(name)->cpIdx;
            emit(Instruction(mkstruct, cpIdx, i));
            restore();
        }
        void emitFunctionCall(astnode* n) {
            if (noisey) cout<<"Compiling Function Call."<<endl;
            SymbolTableEntry fn_info = symTable.lookup(n->left->token.getString(), n->left->token.lineNumber());
            int argsCount = 0;
            for (auto x = n->right; x != nullptr; x = x->next)
                argsCount++;
            genCode(n->right, false);
            genExpression(n->left, false);
            emit(Instruction(call, fn_info.constPoolIndex, argsCount, n->left->token.scopeLevel()));
        }
        void emitListConstructor(astnode* n) {
            emit(Instruction(ldconst, symTable.getConstPool().insert(new deque<StackItem>())));
            for (astnode* it = n->left; it != nullptr; it = it->next) {
                genExpression(it, false);
                emit(Instruction(list_append));
            }
        }
        void emitBlock(astnode* n) {
            int L1 = skipEmit(0);
            emit(Instruction(entblk, L1+2));
            symTable.openFunctionScope(n->token.getString(), L1+2);
            genCode(n->left, false);
            symTable.closeScope();
            emit(Instruction(retblk));
        }
        void emitClassDef(astnode* n) {
            cout<<"Compiling class Definition: "<<n->left->token.getString()<<endl;
            int L1 = skipEmit(0);
            skipEmit(1);
            string name = n->left->token.getString();
            ClassObject* ent = symTable.lookupClass(name);
            emit(Instruction(defstruct, ent->cpIdx, L1+1));
            int cpos = skipEmit(0);
            skipTo(L1);
            emit(Instruction(jump, cpos));
            restore();
        }
        void emitFuncDef(astnode* n) {
            if (noisey) cout<<"Compiling Function Definition: "<<n->token.getString()<<endl;
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
            skipTo(L1);
            emit(Instruction(jump, cpos));
            restore();
            emitStoreFuncInEnvironment(n, false);            
        }
        void emitStoreFuncInEnvironment(astnode* n, bool isLambda) {
            string name = n->token.getString();
            SymbolTableEntry fn_info = symTable.lookup(name, n->token.lineNumber());
            emit(Instruction(mkclosure, fn_info.constPoolIndex, fn_info.depth));
            if (isLambda)
                return;
            if (fn_info.depth == GLOBAL_SCOPE) {
                emit(Instruction(ldaddr, fn_info.addr));
                emit(Instruction(stglobal, fn_info.addr));
            } else {
                emit(Instruction(ldaddr, fn_info.addr));
                emit(Instruction(stlocal, fn_info.addr));
            }
        }
        void emitLet(astnode* n) {
            switch (n->left->expr) {
                case BIN_EXPR:{
                    emitBinaryOperator(n->left); 
                } break;
                case ID_EXPR: {
                    genCode(n->right, false); 
                    genCode(n->left, true);
                } break;
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
        void emitIfStmt(astnode* n) {
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
        void genStatement(astnode* n, bool needLvalue) {
            switch (n->stmt) {
                case DEF_CLASS_STMT: { emitClassDef(n); } break;
                case DEF_STMT:    { emitFuncDef(n); } break;
                case BLOCK_STMT:  { emitBlock(n);   } break;
                case IF_STMT:     { emitIfStmt(n);   } break;
                case LET_STMT:    { emitLet(n);      } break;
                case PRINT_STMT:  { emitPrint(n);      } break;
                case RETURN_STMT: { emitReturn(n);     } break;
                case WHILE_STMT:  { emitWhile(n);      } break;
                case EXPR_STMT:   { genCode(n->left, false); } break;
                default: break;
            };
        }
        void genExpression(astnode* n, bool needLvalue) {
            switch (n->expr) {
                case CONST_EXPR:     { emitConstant(n);  } break;
                case ID_EXPR:        { emitLoad(n, needLvalue); } break;
                case BIN_EXPR:       { emitBinaryOperator(n); } break;
                case UOP_EXPR:       { emitUnaryOperator(n);  } break;
                case LAMBDA_EXPR:    { emitLambda(n);        } break;
                case FUNC_EXPR:      { emitFunctionCall(n);   } break;
                case LISTCON_EXPR:   { emitListConstructor(n); } break;
                case SUBSCRIPT_EXPR: { emitListAccess(n, needLvalue); } break;
                case FIELD_EXPR:     { emitFieldAccess(n, needLvalue); } break;
                case LIST_EXPR:      { emitListOperation(n); } break;
                case BLESS_EXPR:     { emitBlessExpr(n); } break;
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
        void printOperand(StackItem& operand) {
            switch (operand.type) {
                case INTEGER: cout<<to_string(operand.intval); break;
                default: cout<<"."; break;
            }
        }
        void printByteCode() {
            cout<<"Compiled Bytecode: "<<endl;
            int addr = 0;
            for (auto m : code) {
                cout<<setw(2)<<addr<<": [0x"<<setw(2)<<m.op<<" "<<instrStr[m.op]<<" ";
                printOperand(m.operand[0]);
                cout<<" ";
                printOperand(m.operand[1]);
                if (m.op == call) cout<<" "<<m.operand[2].intval;
                cout<<" ]"<<endl;
                if (m.op == halt)
                    break;
                addr++;
            }
            cout<<"Symbol table: ";
            symTable.print();
            cout<<"----------------"<<endl;
        }
    public:
        ByteCodeGenerator() {
            code = vector<Instruction>(255, Instruction(halt, 0));
            cpos = 0;
            highCI = 0;
            noisey = true;
        }
        ConstPool& getConstPool() {
            return symTable.getConstPool();
        }
        vector<Instruction> compile(astnode* n) {
            if (noisey) cout<<"Build Symbol Table: "<<endl;
            sr.buildSymbolTable(n, &symTable);
            rl.resolveLocals(n, &symTable);
            if (noisey) cout<<"Compiling..."<<endl;
            genCode(n, false);
            printByteCode();
            cout<<"Constant Pool: "<<endl;
            for (int i = 0; i < symTable.getConstPool().size(); i++) {
                cout<<i<<": {"<<symTable.getConstPool().get(i).toString()<<"}"<<endl;
            }
            cout<<"-------------------"<<endl;
            return code;
        }
};

#endif