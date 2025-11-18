#ifndef vm_hpp
#define vm_hpp
#include "stackitem.hpp"
#include "bytecodecompiler.hpp"
using namespace std;

static const int MAX_OP_STACK = 255;
static const int MAX_CALL_STACK = 255;



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
            Function* func = pop().closure->func;
            callstk[fp++] = ActivationRecord(func->start_ip, inst.operand[1].intval);
            inparams = true;
        }
        void enterFunction(Instruction& inst) {
            //cout<<"return address: "<<ip<<endl;
            callstk[fp-1].returnAddress = ip;
            ip = inst.operand[0].intval;
            //cout<<"jump to addres: "<<ip<<endl;
            for (int i = callstk[fp-1].num_args; i > 0; i--) {
                callstk[fp-1].locals[i] = pop();
            }
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
            return codePage[ip++];
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
                    StackItem lhs = pop();
                    push(StackItem(lhs.add(rhs)));
                } break;
                case VM_SUB:  {
                    StackItem rhs = pop();
                    StackItem lhs = pop();
                    push(StackItem(lhs.sub(rhs)));
                } break;
                case VM_MUL:  {
                    StackItem rhs = pop();
                    StackItem lhs = pop();
                    push(StackItem(lhs.mul(rhs)));
                } break;
                case VM_DIV:  {
                    StackItem rhs = pop();
                    StackItem lhs = pop();
                    push(StackItem(lhs.div(rhs)));
                } break;
                case VM_MOD:  {
                    StackItem rhs = pop();
                    StackItem lhs = pop();
                    push(StackItem(lhs.mod(rhs)));
                } break;
                case VM_LT:   {
                    StackItem rhs = pop();
                    StackItem lhs = pop();
                    push(lhs.compareTo(rhs));
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
            push(opstk[(MAX_OP_STACK-1) - inst.operand[0].intval]);
        }
        void loadLocal(Instruction& inst) {
            push(callstk[fp-(inparams ? 2:1)].locals[inst.operand[0].intval]);
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