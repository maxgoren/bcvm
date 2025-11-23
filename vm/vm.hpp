#ifndef vm_hpp
#define vm_hpp
#include "stackitem.hpp"
#include "instruction.hpp"
using namespace std;

static const int MAX_OP_STACK = 1255;
static const int MAX_CALL_STACK = 1255;



struct ActivationRecord {
    int returnAddress;
    StackItem locals[255];
    int num_locals;
    int num_args;
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
        BlockScope constPool;
        int ip;
        int sp;
        int fp;
        bool inparams;
        ActivationRecord callstk[MAX_CALL_STACK];
        StackItem opstk[MAX_OP_STACK];
        ActivationRecord& peekAR() {
            return callstk[(fp-inparams)];
        }
        void push(StackItem item) {
            sp = sp < 0 ? 0:sp;
            opstk[++sp] = item;
            if (verbLev > 1)
                cout<<"Push("<<sp<<"): "<<opstk[sp].toString()<<endl;
        }
        StackItem& top() {
            return opstk[sp];
        }
        StackItem pop() {
            if (verbLev > 1)
                cout<<"Pop: "<<opstk[sp].toString()<<endl;
            return opstk[sp--];
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
            StackItem func;
            //cout<<"move args from stack to AR: ";
            for (int i = callstk[fp].num_args; i > 0; i--) {
                callstk[fp].locals[i] = pop();
            }
            func = pop();
            if (func.type != CLOSURE) {
                ip = inst.operand[0].intval;
            } else {
                ip = func.closure->func->start_ip;
            }
            inparams = false;
        }
        void storeGlobal() {
            StackItem t = pop();
            StackItem val = pop();
            callstk[0].locals[t.intval] =  val;
            if (verbLev > 1)
                cout<<"Stored global at "<<t.intval<<endl;
        }
        void loadGlobal(Instruction& inst) {
            if (verbLev > 1)
                cout<<"Load "<<callstk[0].locals[inst.operand[0].intval].toString()<<" from "<<(inst.operand[0].intval)<<endl;
            push(callstk[0].locals[inst.operand[0].intval]);
        }
        void loadLocal(Instruction& inst) {
            push(peekAR().locals[inst.operand[0].intval]);
            if (verbLev > 1)
                cout<<"loaded local from "<<inst.operand[0].intval<<" of scope "<<(fp-(inparams?1:0))<<(inparams ? " as param":" as local")<<endl;
        } 
        void storeLocal(Instruction& inst) {
            StackItem t = pop();
            StackItem val = pop();
            peekAR().locals[t.intval] = val;
            if (verbLev > 1)
                cout<<"Stored local at "<<t.intval<<" in scope "<<(fp-(inparams?1:0))<<(inparams ? " as param":" as local")<<endl;
        }
        void loadIndexed(Instruction& inst) {
            StackItem t = pop();
            push(pop().list->at(t.numval));
        }
        void indexed_store(Instruction& inst) {
            StackItem index = pop();
            StackItem list = pop();
            list.list->at(index.numval) = pop();
            push(list);
            push(index);
        }
        void loadConst(Instruction& inst) {
            push(inst.operand[0]);
        }
        void branchOnFalse(Instruction& inst) {
            bool tmp = pop().boolval;
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
                    StackItem rhs = pop();
                    StackItem lhs = pop();
                    push(StackItem(lhs.compareTo(rhs)));
                } break;
                case VM_GT:   {
                    StackItem rhs = pop();
                    StackItem lhs = pop();
                    push(StackItem(rhs.compareTo(lhs)));
                } break;
                case VM_EQU:  {
                    StackItem rhs = pop();
                    StackItem lhs = pop();
                    push(StackItem(!lhs.compareTo(rhs) && !rhs.compareTo(lhs)));
                } break;
                case VM_NEQ: {
                    StackItem rhs = pop();
                    StackItem lhs = pop();
                    push(StackItem(lhs.compareTo(rhs) || rhs.compareTo(lhs)));
                } break;
            }
        }
        void arithmeticOperation(Instruction& inst) {
            switch (inst.operand[0].intval) {
                case VM_ADD:  {
                    StackItem rhs = pop();
                    top().add(rhs);
                } break;
                case VM_SUB:  {
                    StackItem rhs = pop();
                    top().sub(rhs);
                } break;
                case VM_MUL:  {
                    StackItem rhs = pop();
                    top().mul(rhs);
                } break;
                case VM_DIV:  {
                    StackItem rhs = pop();
                    top().div(rhs);
                } break;
                case VM_MOD:  {
                    StackItem rhs = pop();
                    top().mod(rhs);
                } break;
                default:
                    break;
            }
        }
        void appendList() {
            StackItem item = pop();
            if (top().type == LIST)
                top().list->push_back(item);
        }
        void pushList() {
            StackItem item = pop();
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
                case list_len:  { push((double)pop().list->size()); } break;
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
                case ldlocaladdr:  { push(inst.operand[0].intval); } break;
                case ldglobaladdr: { push(inst.operand[0].intval); } break;
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
            cout<<pop().toString()<<endl;
        }
        void printOperandStack() {
            cout<<"Operands:  ";
            for (int i = 0; i < sp; i++) {
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