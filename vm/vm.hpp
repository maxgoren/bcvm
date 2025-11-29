#ifndef vm_hpp
#define vm_hpp
#include <vector>
#include "stackitem.hpp"
#include "instruction.hpp"
using namespace std;

static const int MAX_OP_STACK = 1255;
static const int MAX_CALL_STACK = 1255;

struct ActivationRecord {
    int num_args;
    int num_locals;
    int returnAddress;
    StackItem locals[255];
    ActivationRecord* control;
    ActivationRecord* access;
    ActivationRecord(int ra = 0, int numArgs = 0, ActivationRecord* calling = nullptr, ActivationRecord* defining = nullptr) {
        returnAddress = ra;
        num_args = numArgs;
        control = calling;
        access = defining;
    }
};



class VM {
    private:
        bool running = false;
        int verbLev;
        Instruction haltSentinel;
        vector<Instruction> codePage;
        int ip;
        int sp;
        int fp;
        ConstPool constPool;
        ActivationRecord *callstk;
        ActivationRecord *globals;
        StackItem opstk[MAX_OP_STACK];
        ActivationRecord* walkChain(int d) {
            if (d == -56) return globals;
            auto x = callstk;
            while (x != nullptr && d > 0) {
                x = x->access;
                d--;
            }
            if (x == nullptr) {
                x = globals;
                cout<<"Wrong chain length, default to globals."<<endl;
            } else {
                cout << "WalkChain depth " << d << ", returns AR at ";
                for (int i = 0; i <= 5; i++) {
                    cout<<"{i: "<<x->locals[i].toString()<<"}";
                }
            }
            cout<<endl;
            return x ? x:globals;
        }
        StackItem& top() {
            return opstk[sp];
        }
        void closeOver(Instruction& inst) {
            int func_id = inst.operand[0].intval;
            int depth = inst.operand[1].intval;
            cout<<"Closing over "<<depth<<" defining env's"<<endl;
            //stash current activation record as closures defining env
            constPool.get(func_id).objval->closure->env = callstk; 
            opstk[++sp] = constPool.get(func_id);
        }
        void closeScope() {
            if (callstk == nullptr) {
                for (int i = 0; i < 8; i++)
                    cout<<"Hey what the fuck is this?"<<endl;
            }
            ip = callstk->returnAddress;
            popScope();
        }
        void openBlock(Instruction& inst) {
            callstk = new ActivationRecord(ip, inst.operand[1].intval, callstk, walkChain(inst.operand[2].intval));
        }
        void popScope() {
            if (callstk != nullptr && callstk->control != nullptr)
                callstk = callstk->control;
            if (verbLev > 1)
                cout<<"Leaving scope."<<endl;
        }
        void callProcedure(Instruction& inst) {
            int returnAddr = ip;
            int numArgs = inst.operand[1].intval;
            int lexDepth = inst.operand[2].intval;
            int cpIdx = inst.operand[0].intval;
            Closure* close;
            if (cpIdx == -1) {
                close = opstk[sp--].objval->closure;
            } else {
                close = constPool.get(cpIdx).objval->closure;
            }
            ActivationRecord* staticLink;
            if (lexDepth == -1) {
                staticLink = globals;
            } else if (lexDepth == 0) {
                staticLink = callstk;
            } else {
                staticLink = close->env;
            }
            ActivationRecord* nextAR = new ActivationRecord(returnAddr, numArgs, callstk, staticLink);            
            for (int i = numArgs; i > 0; i--) {
                nextAR->locals[i] = opstk[sp--];
            }
            nextAR->access = opstk[sp--].objval->closure->env;
            nextAR->returnAddress = returnAddr;
            callstk = nextAR;
            ip = close->func->start_ip;
        }
        void storeGlobal() {
            StackItem t = opstk[sp--];
            StackItem val = opstk[sp--];
            globals->locals[t.intval] =  val;
            if (verbLev > 1)
                cout<<"Stored global at "<<t.intval<<endl;
        }
        void loadGlobal(Instruction& inst) {
            if (verbLev > 1)
                cout<<"Load "<<globals->locals[inst.operand[0].intval].toString()<<" from "<<(inst.operand[0].intval)<<endl;
            opstk[++sp] = globals->locals[inst.operand[0].intval];
        }
        void loadLocal(Instruction& inst) {
            opstk[++sp] = callstk->locals[inst.operand[0].intval];
            if (verbLev > 1)
                cout<<"loaded local: "<<opstk[sp].toString()<<endl;
        } 
        void loadUpval(Instruction& inst) {
            opstk[++sp] = walkChain(inst.operand[1].intval)->locals[inst.operand[0].intval];
            if (verbLev > 1)
                cout<<"loaded vpval: "<<opstk[sp].toString()<<"from "<<inst.operand[0].intval<<" of scope "<<(fp)<<endl;
        } 
        void storeLocal(Instruction& inst) {
            StackItem t = opstk[sp--];
            StackItem val = opstk[sp--];
            callstk->locals[t.intval] = val;
            if (verbLev > 1)
                cout<<"Stored local at "<<t.intval<<endl;
        }
        void storeUpval(Instruction& inst) {
            StackItem t = opstk[sp--];
            StackItem val = opstk[sp--];
            walkChain(inst.operand[0].intval)->locals[t.intval] = val;
            if (verbLev > 1)
                cout<<"Stored upval at "<<t.intval<<" in scope "<<(inst.operand[0].intval)<<endl;
        }
        void loadIndexed(Instruction& inst) {
            StackItem t = opstk[sp--];
            opstk[++sp] = (opstk[sp--].objval->list->at(t.numval));
        }
        void indexed_store(Instruction& inst) {
            StackItem index = opstk[sp--];
            StackItem list = opstk[sp--];
            list.objval->list->at(index.numval) = opstk[sp--];
            opstk[++sp] = (list);
            opstk[++sp] = (index);
        }
        void loadConst(Instruction& inst) {
            if (inst.operand[0].type == INTEGER) {
                opstk[++sp] = (constPool.get(inst.operand[0].intval));
            } else {
                opstk[++sp] = (inst.operand[0]);
            }
        }
        void branchOnFalse(Instruction& inst) {
            bool tmp = opstk[sp--].boolval;
            if (tmp == false) {
                ip = inst.operand[0].intval;
            }
        }
        void uncondBranch(Instruction& inst) {
            ip = inst.operand[0].intval;
        }
        void unaryOperation(Instruction& inst) {
            switch (inst.operand[0].intval) {
                case VM_NEG: { 
                    switch (top().type) {
                        case INTEGER: top().intval = -top().intval; break;
                        case NUMBER:  top().numval = -top().numval; break;
                        case BOOLEAN: top().boolval = !top().boolval; break;
                    }
                } break;
            }
        }
        void binaryOperation(Instruction& inst) {
            if (inst.operand[0].intval > 6) {
                relationOperation(inst);
            } else {
                arithmeticOperation(inst);
            }
        }
        void relationOperation(Instruction& inst) {
            switch (inst.operand[0].intval) {
                case VM_LT:   {
                    StackItem rhs = opstk[sp--];
                    StackItem lhs = opstk[sp--];
                    opstk[++sp] = (StackItem(lhs.lessThan(rhs)));
                } break;
                case VM_GT:   {
                    StackItem rhs = opstk[sp--];
                    StackItem lhs = opstk[sp--];
                    opstk[++sp] = (StackItem(rhs.lessThan(lhs)));
                } break;
                case VM_EQU:  {
                    StackItem rhs = opstk[sp--];
                    StackItem lhs = opstk[sp--];
                    opstk[++sp] = (StackItem(!lhs.lessThan(rhs) && !rhs.lessThan(lhs)));
                } break;
                case VM_NEQ: {
                    StackItem rhs = opstk[sp--];
                    StackItem lhs = opstk[sp--];
                    opstk[++sp] = (StackItem(lhs.lessThan(rhs) || rhs.lessThan(lhs)));
                } break;
            }
        }
        void arithmeticOperation(Instruction& inst) {
            switch (inst.operand[0].intval) {
                case VM_ADD:  {
                    StackItem rhs = opstk[sp--];
                    top().add(rhs);
                } break;
                case VM_SUB:  {
                    StackItem rhs = opstk[sp--];
                    top().sub(rhs);
                } break;
                case VM_MUL:  {
                    StackItem rhs = opstk[sp--];
                    top().mul(rhs);
                } break;
                case VM_DIV:  {
                    StackItem rhs = opstk[sp--];
                    top().div(rhs);
                } break;
                case VM_MOD:  {
                    StackItem rhs = opstk[sp--];
                    top().mod(rhs);
                } break;
                default:
                    break;
            }
        }
        void appendList() {
            StackItem item = opstk[sp--];
            if (top().type == OBJECT && top().objval->type == LIST)
                top().objval->list->push_back(item);
        }
        void pushList() {
            StackItem item = opstk[sp--];
            if (top().type == OBJECT && top().objval->type == LIST)
                top().objval->list->push_front(item);
        }
        void haltvm() {
            running = false;
        }
        void execute(Instruction& inst) {
            switch (inst.op) {
                case list_append: { appendList();   } break;
                case list_push:  { pushList(); } break;
                case list_len:  { opstk[++sp] = ((double)opstk[sp--].objval->list->size()); } break;
                case call:   { callProcedure(inst); } break;
                case retfun:   { closeScope(); } break;
                case entblk:   { openBlock(inst); } break;
                case retblk:   { popScope(); } break;
                case jump:     { uncondBranch(inst); } break;
                case brf:      { branchOnFalse(inst); } break;
                case binop:    { binaryOperation(inst); } break;
                case unop:     { unaryOperation(inst); } break;
                case print:    { printTopOfStack(); } break;
                case halt:     { haltvm(); } break;
                case stglobal: { storeGlobal(); } break;
                case stupval:  { storeUpval(inst); } break;
                case stlocal:  { storeLocal(inst); } break;
                case stfield:  { indexed_store(inst); } break;
                case ldconst:  { loadConst(inst); } break;
                case ldglobal: { loadGlobal(inst); } break;
                case ldupval:  { loadUpval(inst); } break;
                case ldlocal:  { loadLocal(inst); } break;
                case ldlocaladdr:  { opstk[++sp] = (inst.operand[0].intval); } break;
                case ldglobaladdr: { opstk[++sp] = (inst.operand[0].intval); } break;
                case ldfield:      { loadIndexed(inst); } break;
                case label: { /* nop() */ } break;
                case mkclosure: { closeOver(inst); } break;
                default:
                    break;
            }       
        }
        Instruction& fetch() {
            return ip < codePage.size() && ip > -1 ? codePage[ip++]:haltSentinel;
        }
        void printInstruction(Instruction& inst) {
            cout<<"Instrctn: "<<ip<<": [0x0"<<inst.op<<"("<<instrStr[inst.op]<<"), "<<inst.operand[0].toString()<<","<<inst.operand[1].toString();
            if (inst.op == call) cout<<", "<<inst.operand[2].intval;
            cout<<"]  \n";
        }
        void printTopOfStack() {
            cout<<opstk[sp--].toString()<<endl;
        }
        void printOperandStack() {
            cout<<"Operands:  ";
            for (int i = 0; i <= sp; i++) {
                cout<<i<<": ["<<opstk[i].toString()<<"] ";
            }
            cout<<endl;
        }
        void printCallStack() {
            cout<<"Callstack: \n";
            auto x = callstk;
            int i = 0;
            while (x != nullptr) {
                cout<<"\t   "<<i++<<": [ ";
                for (int j = 1; j <= 5; j++) {
                    cout<<(j)<<": "<<"{"<<x->locals[j].toString()<<"}, ";
                }
                cout<<"]"<<endl;
                if (x->access != x->control) {
                    cout<<"\t Lexical stack: \n";
                    int l = i;
                    auto t = x->access;
                    while (t != nullptr) {
                        cout<<"\t \t  "<<l++<<": [ ";
                        for (int j = 1; j <= 5; j++) {
                            cout<<(j)<<": "<<"{"<<t->locals[j].toString()<<"}, ";
                        }
                        cout<<"]"<<endl;
                        t = t->access;
                    }
                }
                x = x->control;
            }            
            cout<<"Globals: \n";
            cout<<"\t    ";
            for (int j = 1; j <= 5; j++) {
                cout<<(j)<<": "<<"{"<<globals->locals[j].toString()<<"}, ";
            }
            cout<<"]"<<endl;
        }
        void init(vector<Instruction>& cp, int verbosity) {
            codePage = cp;
            if (ip > 0) ip -= 1;
            verbLev = verbosity;
        }
    public:
        VM() {
            ip = 0;
            sp = 0;
            fp = 0;
            haltSentinel = Instruction(halt);
            globals = new ActivationRecord(0, 0, nullptr, nullptr);
            callstk = globals;
        }
        void setConstPool(ConstPool& cp) {
            constPool = cp;
        }
        void run(vector<Instruction>& cp, int verbosity) {
            init(cp, verbosity);
            running = true;
            while (running) {
                Instruction inst = fetch();
                if (verbosity > 0) {
                    printInstruction(inst);
                    cout<<"----------------"<<endl;
                }
                execute(inst);
                if (verbosity > 1) {
                    cout<<"----------------"<<endl;                
                    printOperandStack();
                }
                if (verbosity > 2) {
                    printCallStack();
                    cout<<"================"<<endl;
                }
            }
        }
};


#endif