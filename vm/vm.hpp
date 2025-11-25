#ifndef vm_hpp
#define vm_hpp
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
    ActivationRecord(int ra = 0, int numArgs = 0) {
        returnAddress = ra;
        num_args = numArgs;
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
        bool inparams;
        ConstPool constPool;
        ActivationRecord *callstk; //[MAX_CALL_STACK];
        ActivationRecord *globals;
        StackItem opstk[MAX_OP_STACK];
        ActivationRecord& peekAR() {
            return callstk[(fp-inparams)];
        }
        StackItem& top() {
            return opstk[sp];
        }
        void openScope(Instruction& inst) {
            if (verbLev > 1)
                cout<<"Opening Scope with "<<inst.operand[1].intval<<" args and "<<inst.operand[2].intval<<"locals"<<endl;
            openBlock(inst);
            inparams = true;
        }
        void closeScope() {
            ip = callstk[fp].returnAddress;
            popScope();
        }
        void openBlock(Instruction& inst) {
            callstk[++fp] = ActivationRecord(ip, inst.operand[1].intval);
        }
        void popScope() {
            fp--;
            if (verbLev > 1)
                cout<<"Leaving scope."<<endl;
        }
        void enterFunction(Instruction& inst) {
            callstk[fp].returnAddress = ip;
            //cout<<"jump to addres: "<<ip<<endl;
            //cout<<"move args from stack to AR: ";
            for (int i = inst.operand[1].intval; i > 0; i--) {
                callstk[fp].locals[i] = opstk[sp--];
            }
            StackItem func = opstk[sp--];
            //cout<<"Pulled from top of stack: "<<func.toString()<<endl;
            if (inst.operand[0].intval != -1) {
                ip = constPool.get(inst.operand[0].intval).closure->func->start_ip;
            } else {
                ip = func.closure->func->start_ip;
            }
            inparams = false;
        }
        void storeGlobal() {
            StackItem t = opstk[sp--];
            StackItem val = opstk[sp--];
            callstk[0].locals[t.intval] =  val;
            if (verbLev > 1)
                cout<<"Stored global at "<<t.intval<<endl;
        }
        void loadGlobal(Instruction& inst) {
            if (verbLev > 1)
                cout<<"Load "<<callstk[0].locals[inst.operand[0].intval].toString()<<" from "<<(inst.operand[0].intval)<<endl;
            opstk[++sp] = (callstk[0].locals[inst.operand[0].intval]);
        }
        void loadLocal(Instruction& inst) {
            opstk[++sp] = (peekAR().locals[inst.operand[0].intval]);
            if (verbLev > 1)
                cout<<"loaded local from "<<inst.operand[0].intval<<" of scope "<<(fp-(inparams?1:0))<<(inparams ? " as param":" as local")<<endl;
        } 
        void storeLocal(Instruction& inst) {
            StackItem t = opstk[sp--];
            StackItem val = opstk[sp--];
            peekAR().locals[t.intval] = val;
            if (verbLev > 1)
                cout<<"Stored local at "<<t.intval<<" in scope "<<(fp-(inparams?1:0))<<(inparams ? " as param":" as local")<<endl;
        }
        void loadIndexed(Instruction& inst) {
            StackItem t = opstk[sp--];
            opstk[++sp] = (opstk[sp--].list->at(t.numval));
        }
        void indexed_store(Instruction& inst) {
            StackItem index = opstk[sp--];
            StackItem list = opstk[sp--];
            list.list->at(index.numval) = opstk[sp--];
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
                    opstk[++sp] = (StackItem(lhs.compareTo(rhs)));
                } break;
                case VM_GT:   {
                    StackItem rhs = opstk[sp--];
                    StackItem lhs = opstk[sp--];
                    opstk[++sp] = (StackItem(rhs.compareTo(lhs)));
                } break;
                case VM_EQU:  {
                    StackItem rhs = opstk[sp--];
                    StackItem lhs = opstk[sp--];
                    opstk[++sp] = (StackItem(!lhs.compareTo(rhs) && !rhs.compareTo(lhs)));
                } break;
                case VM_NEQ: {
                    StackItem rhs = opstk[sp--];
                    StackItem lhs = opstk[sp--];
                    opstk[++sp] = (StackItem(lhs.compareTo(rhs) || rhs.compareTo(lhs)));
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
            if (top().type == LIST)
                top().list->push_back(item);
        }
        void pushList() {
            StackItem item = opstk[sp--];
            if (top().type == LIST)
                top().list->push_front(item);
        }
        void haltvm() {
            running = false;
        }
        void execute(Instruction& inst) {
            switch (inst.op) {
                case list_append: { appendList();   } break;
                case list_push:  { pushList(); } break;
                case list_len:  { opstk[++sp] = ((double)opstk[sp--].list->size()); } break;
                case call:     { openScope(inst); } break;
                case entfun:   { enterFunction(inst); } break;
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
                case stlocal:  { storeLocal(inst); } break;
                case stfield:  { indexed_store(inst); } break;
                case ldconst:  { loadConst(inst); } break;
                case ldglobal: { loadGlobal(inst); } break;
                case ldlocal:  { loadLocal(inst); } break;
                case ldlocaladdr:  { opstk[++sp] = (inst.operand[0].intval); } break;
                case ldglobaladdr: { opstk[++sp] = (inst.operand[0].intval); } break;
                case ldfield:      { loadIndexed(inst); } break;
                case label: { /* nop() */ } break;
                default:
                    break;
            }       
        }
        Instruction& fetch() {
            return ip < codePage.size() && ip > -1 ?codePage[ip++]:haltSentinel;
        }
        void printInstruction(Instruction& inst) {
            cout<<"Instrctn: "<<ip<<": [0x0"<<inst.op<<"("<<instrStr[inst.op]<<"), "<<inst.operand[0].toString()<<","<<inst.operand[1].toString()<<"]  \n";
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
            for (int i = fp-1; i >= 0; i--) {
                cout<<"\t   "<<i<<": [ ";
                for (int j = 1; j <= 5; j++) {
                    cout<<(j)<<": "<<"{"<<callstk[i].locals[j].toString()<<"}, ";
                }
                cout<<"]"<<endl;
            }
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
            callstk[fp++] = ActivationRecord(MAX_CALL_STACK, 0);
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