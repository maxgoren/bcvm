#ifndef vm_hpp
#define vm_hpp
#include "stackitem.hpp"
#include "bytecodecompiler.hpp"
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
        int ip;
        int sp;
        int fp;
        bool inparams;
        ActivationRecord callstk[MAX_CALL_STACK];
        StackItem opstk[MAX_OP_STACK];
        void push(StackItem item) {
            sp = sp < 0 ? 0:sp;
            opstk[sp++] = item;
            if (verbLev > 1)
                cout<<"Push("<<sp-1<<"): "<<opstk[sp-1].toString()<<endl;
        }
        StackItem& top() {
            return opstk[sp-1];
        }
        StackItem pop() {
            if (verbLev > 1)
                cout<<"Pop: "<<opstk[sp-1].toString()<<endl;
            return opstk[--sp];
        }
        void openScope(Instruction& inst) {
            if (verbLev > 1)
                cout<<"Opening Scope with "<<inst.operand[1].intval<<" args and "<<inst.operand[2].intval<<"locals"<<endl;
            callstk[fp++] = ActivationRecord(ip, inst.operand[1].intval);
            inparams = true;
        }
        void enterFunction(Instruction& inst) {
            callstk[fp-1].returnAddress = ip;
            //cout<<"jump to addres: "<<ip<<endl;
            StackItem func;
            //cout<<"move args from stack to AR: ";
            for (int i = callstk[fp-1].num_args; i > 0; i--) {
                callstk[fp-1].locals[i] = pop();
            }
            //cout<<"popp function"<<endl;
            if (sp > 0) func = pop();
            if (func.type != CLOSURE) ip =  inst.operand[0].intval;
            else ip = func.closure->func->start_ip;
            inparams = false;
        }
        void closeScope() {
            ip = callstk[fp-1].returnAddress;
            fp--;
            if (verbLev > 1)
                cout<<"Leaving scope."<<endl;
        }
        void global_store() {
            StackItem t = pop();
            StackItem val = pop();
            opstk[t.intval] =  val;
            if (verbLev > 1)
                cout<<"Stored global at "<<t.intval<<endl;
        }
        void local_store(int depth) {
            StackItem t = pop();
            StackItem val = pop();
            callstk[depth].locals[t.intval] = val;
            if (verbLev > 1)
                cout<<"Stored local at "<<t.intval<<" in scope "<<depth<<endl;
        }
        Instruction& fetch() {
            return ip < codePage.size() && ip > -1 ?codePage[ip++]:haltSentinel;
        }
        void printInstruction(Instruction& inst) {
            cout<<ip<<": [0x0"<<inst.op<<"("<<instrStr[inst.op]<<"), "<<inst.operand[0].toString()<<","<<inst.operand[1].toString()<<"]  \n";
        }
        void branchOnFalse(Instruction& inst) {
            bool tmp = pop().boolval;
            if (tmp == false) {
                ip = inst.operand[0].intval;
            }
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
                default:
                    break;
            }
        }
        void loadConst(Instruction& inst) {
            push(inst.operand[0]);
        }
        void loadGlobal(Instruction& inst) {
            if (verbLev > 1)
                cout<<"Load "<<opstk[(MAX_OP_STACK-1) - inst.operand[0].intval].toString()<<" from "<<((MAX_OP_STACK-1) - inst.operand[0].intval)<<endl;
            push(opstk[(MAX_OP_STACK-1) - inst.operand[0].intval]);
        }
        void loadLocal(Instruction& inst) {
            push(callstk[fp-(inparams ? 2:1)].locals[inst.operand[0].intval]);
        }
        void appendList() {
            StackItem item = pop();
            if (top().type == LIST)
                top().list->push_back(item);
        }
        void firstList() {
            if (top().type == LIST) 
                push(top().list->front()); 
        }
        void printTopOfStack() {
            cout<<pop().toString()<<endl;
        }
        void haltvm() {
            running = false;
        }
        void uncondBranch(Instruction& inst) {
            ip = inst.operand[0].intval;
        }
        void execute(Instruction& inst) {
            switch (inst.op) {
                case append:   {  appendList();   } break;
                case first:    {  firstList();  } break;
                case rest:     {    } break;
                case call:     { openScope(inst); } break;
                case entfun:   { enterFunction(inst); } break;
                case retfun:   { closeScope(); } break;
                case jump:     { uncondBranch(inst); } break;
                case brf:      { branchOnFalse(inst); } break;
                case binop:    { binaryOperation(inst); } break;
                case unop:     { unaryOperation(inst); } break;
                case print:    { printTopOfStack(); } break;
                case halt:     { haltvm(); } break;
                case stglobal: { global_store(); } break;
                case stlocal:  { local_store(inst.operand[0].intval); } break;
                case ldconst:  { loadConst(inst); } break;
                case ldglobal: { loadGlobal(inst); } break;
                case ldlocal:  { loadLocal(inst); } break;
                case ldlocaladdr:  { push(inst.operand[0].intval); } break;
                case ldglobaladdr: { push((MAX_OP_STACK-1) - inst.operand[0].intval); } break;
                case ldfield:      { 
                    StackItem t = pop();
                    push(top().list->at(t.numval));
                } break;
                case label: { /* nop() */ } break;
                default:
                    break;
            }       
        }
        void printOperandStack() {
            for (int i = 0; i < sp; i++) {
                cout<<"["<<opstk[i].toString()<<"] ";
            }
            cout<<endl;
        }
        void printCallStack() {
            for (int i = fp-1; i >= 0; i--) {
                cout<<i<<": [ ";
                for (int j = 0; j <= callstk[i].num_args; j++) {
                    cout<<"{"<<j<<": "<<callstk[i].locals[j].toString()<<"} ";
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
        }
        void run(vector<Instruction>& cp, int verbosity) {
            init(cp, verbosity);
            running = true;
            while (running) {
                Instruction inst = fetch();
                if (verbosity > 0)
                    printInstruction(inst);
                execute(inst);
                //cout<<"----------------"<<endl;
                if (verbosity > 1)
                    printOperandStack();
                if (verbosity > 2)
                    printCallStack();
            }
        }
};


#endif