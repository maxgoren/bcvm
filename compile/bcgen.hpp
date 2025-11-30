#ifndef bytecodecompiler_hpp
#define bytecodecompiler_hpp
#include <iomanip>
#include <unordered_map>
#include "../parse/ast.hpp"
#include "../vm/instruction.hpp"
#include "scopingst.hpp"
using namespace std;


//{ fn ok() { println "hi"; }; ok(); }


const int GLOBAL_SCOPE = -1;

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
                    buildSymbolTable(t->next);
                    return;
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
                    string name = nameLambda();
                    t->token.setString(name);
                    symTable->openFunctionScope(t->token.getString(), -1);
                    buildSymbolTable(t->left);
                    buildSymbolTable(t->right);
                    symTable->closeScope();
                    buildSymbolTable(t->next);
                    return;
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
        STBuilder() { }
        void buildSymbolTable(astnode* ast, ScopingST* st) {
            symTable = st;
            buildSymbolTable(ast);
        }
};

class ResolveLocals {
    private:
        int scope;
        vector<unordered_map<string, bool>> scopes;
        void openScope() {
            scopes.push_back(unordered_map<string,bool>());
            cout<<"Open new scope"<<endl;
        }
        void closeScope() {
            scopes.pop_back();
            cout<<"Close scope."<<endl;
        }
        void declareName(string name) {
            if (scopes.empty())
                return;
            if (scopes.back().find(name) != scopes.back().end()) {
                cout<<"A variable with the name "<<name<<" has already been declared."<<endl;
            }
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
                    cout << "Variable " << name << " resolved to scope level " << t->token.scopeLevel() <<" ("<<(scopes.size()-1-i)<<")"<<endl;
                    return;
                }
            }
            cout << "Variable " << name << " resolved to scope level as global"<< endl;
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
                    openScope();
                    resolve(node->left);
                    closeScope();
                } break;
                case DEF_STMT: {
                    declareName(node->token.getString());
                    defineName(node->token.getString());
                    openScope();
                    for (auto it = node->left; it != nullptr; it = it->next) {
                        declareName(it->left->token.getString());
                        defineName(it->left->token.getString());
                    }
                    resolve(node->right);
                    closeScope();
                } break;
            }
        }
        void resolveExpression(astnode* node) {
            switch (node->expr) {
                case ID_EXPR: resolveName(node->token.getString(), node); break;
                case FUNC_EXPR: {
                    resolveName(node->left->token.getString(), node->left); 
                    for (auto it = node->right; it != nullptr; it = it->next)
                        resolve(it);
                    return;   
                } break;
                case LAMBDA_EXPR: {
                    openScope();
                    for (auto it = node->left; it != nullptr; it = it->next) {
                        resolve(it);
                    }
                    resolve(node->right);
                    closeScope();
                    return;
                }break;
                default:
                    break;
            }
            resolve(node->left);
            resolve(node->right);
        }
    public:
        ResolveLocals() {

        }
        void resolve(astnode* node) {
            if (node == nullptr)
                return;
            if (node->kind == STMTNODE) resolveStatement(node);
            else resolveExpression(node);
            resolve(node->next);
        }
};

class  ByteCodeGenerator {
    private:
        vector<Instruction> code;
        int cpos;
        int highCI;
        ScopingST symTable;
        string nextLabel() {
            static int next = 0;
            return "L" + to_string(next++);
        }
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
                cout<<"Compiling Assignment: "<<endl;
                genCode(n->right, false);
                genCode(n->left, true);
                emitStore(n);
            } else {
                cout<<"Compiling BinOp: "<<n->token.getString()<<endl;
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
        void emitLoadAddress(SymbolTableEntry& item, astnode* n) {
            if (item.depth == GLOBAL_SCOPE) {
                emit(Instruction(ldglobaladdr, item.addr));
            } else {
                emit(Instruction(ldlocaladdr, item.addr));
            }
        }
        void emitLoad(astnode* n, bool needLvalue) {
            cout<<"Compiling ID expression: ";
            SymbolTableEntry item = lookup(n->token.getString(), n->token.lineNumber());
            int depth = n->token.scopeLevel();
            if (needLvalue) {
                emitLoadAddress(item, n);
            } else {
                if (item.depth == GLOBAL_SCOPE || depth == GLOBAL_SCOPE) {
                    emit(Instruction(ldglobal, item.addr));
                    cout << "LDGLOBAL: " << n->token.getString()<<"scopelevel="<<n->token.scopeLevel() << " depth=" << n->token.scopeLevel()<< endl;
                } else if (depth == 0) {
                    emit(Instruction(ldlocal, item.addr, 0));
                    cout << "LDLOCAL: " << n->token.getString()<<"scopelevel="<<n->token.scopeLevel() << " depth=" << n->token.scopeLevel()<< endl;
                } else {
                    emit(Instruction(ldupval, item.addr, depth));
                    cout<<"\n [AS UPVAL THO]";
                    cout << "LDUPVAL: " << n->token.getString()<<"scopelevel="<<n->token.scopeLevel() << " depth=" << n->token.scopeLevel()<< endl;
                }
            }
        }
        void emitStore(astnode* node) {
            SymbolTableEntry item = symTable.lookup(node->left->token.getString(), 1);
            int depth = node->left->token.scopeLevel();
            if (item.depth == GLOBAL_SCOPE) {
                emit(Instruction(stglobal, item.addr));
            } else if (depth == 0) {
                emit(Instruction(stlocal, item.addr));
            } else {
                emit(Instruction(stupval, item.addr, depth));
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
            cout<<"Compiling Print Statement: "<<endl;
            genCode(n->left, false); 
            emit(Instruction(print));
        }
        void emitConstant(astnode* n) {
            cout<<"Compiling Constant: "<<n->token.getString()<<endl;
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
            emit(Instruction(label, nextLabel()));
            skipTo(L1);
            emit(Instruction(jump, cpos));
            restore();
            emitStoreFuncInEnvironment(n, true);
        }
        void emitFunctionCall(astnode* n) {
            cout<<"Compiling Function Call."<<endl;
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
        void emitFuncDef(astnode* n) {
            cout<<"Compiling Function Definition: "<<n->token.getString()<<endl;
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
            emitStoreFuncInEnvironment(n, false);            
        }
        void emitStoreFuncInEnvironment(astnode* n, bool isLambda) {
            string name = n->token.getString();
            SymbolTableEntry fn_info = symTable.lookup(name, n->token.lineNumber());
            emit(Instruction(mkclosure, fn_info.constPoolIndex, fn_info.depth));
            if (isLambda)
                return;
            if (fn_info.depth == GLOBAL_SCOPE) {
                emit(Instruction(ldglobaladdr, fn_info.addr, fn_info.depth));
                emit(Instruction(stglobal, fn_info.addr));
            } else {
                emit(Instruction(ldlocaladdr, fn_info.addr, symTable.depth() - fn_info.depth));
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
        void genStatement(astnode* n, bool needLvalue) {
            switch (n->stmt) {
                case DEF_STMT:    { emitFuncDef(n); } break;
                case BLOCK_STMT:  { emitBlock(n);   } break;
                case IF_STMT:     { defineIfStmt(n);   } break;
                case LET_STMT:    { 
                    switch (n->left->expr) {
                        case BIN_EXPR: emitBinaryOperator(n->left); break;
                        case ID_EXPR: {
                            genCode(n->right, false); 
                            genCode(n->left, true);
                        } break;
                    }  
                } break;
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
        STBuilder sr;
        void printOperand(StackItem& operand) {
            switch (operand.type) {
                case INTEGER: cout<<to_string(operand.intval); break;
                default: cout<<"."; break;
            }
        }
    public:
        ByteCodeGenerator() {
            code = vector<Instruction>(255, Instruction(halt, 0));
            cpos = 0;
            highCI = 0;
        }
        ConstPool& getConstPool() {
            return symTable.getConstPool();
        }
        ResolveLocals rl;
        vector<Instruction> compile(astnode* n) {
            cout<<"Build Symbol Table: "<<endl;
            sr.buildSymbolTable(n, &symTable);
            rl.resolve(n);
            cout<<"Compiling..."<<endl;
            genCode(n, false);
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
            return code;
        }
};

#endif