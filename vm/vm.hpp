#ifndef vm_hpp
#define vm_hpp
#include "gc.hpp"
using namespace std;

const int BLOCK_CPIDX = -420;

class VM {
    private:
        friend class GarbageCollector;
        bool running = false;
        int verbLev;
        Instruction haltSentinel;
        vector<Instruction> codePage;
        int ip;
        int sp;
        int fp;
        ConstPool constPool;
        GarbageCollector collector;
        ActivationRecord* callstk;
        ActivationRecord* globals;
        StackItem opstk[MAX_OP_STACK];
        ActivationRecord* walkChain(int d) {
            if (d == GLOBAL_SCOPE) return globals;
            if (d == LOCAL_SCOPE) return callstk;
            auto x = callstk;
            while (x != nullptr && d > 0) {
                x = x->access;
                d--;
            }
            return x;
        }
        StackItem& top(int depth = 0) {
            return opstk[sp-depth];
        }
        ActivationRecord* mostRecentAR(int func_id) {
            auto x = callstk;
            while (x != nullptr) {
                if (x->cp_index == func_id) {
                    break;
                }
                x = x->access;
            }
            return (x == nullptr) ? callstk:x;
        }
        void closeOver(Instruction& inst) {
            int func_id = inst.operand[0].intval;
            auto funcobj = constPool.get(func_id);
            if (funcobj.type == OBJECT && funcobj.objval->type == CLOSURE) {
                auto func = funcobj.objval->closure->func;
                auto env = mostRecentAR(func_id);
                opstk[++sp] = StackItem(alloc.alloc(new Closure(func, env)));
            } else {
                cout<<"ERROR: THAT AINT RIGHT"<<endl;
                running = false;
            }
        }
        void openBlock(Instruction& inst) {
            callstk = new ActivationRecord(BLOCK_CPIDX, ip, callstk, callstk);
        }
        void closeBlock() {
            if (callstk != nullptr && callstk->control != nullptr) {
                callstk = callstk->control;
            }
            if (verbLev > 1)
                cout<<"Leaving scope."<<endl;
        }
        void callProcedure(Instruction& inst) {
            int numArgs = inst.operand[1].intval;
            int cpIdx = inst.operand[0].intval;
            if (opstk[sp].type == OBJECT && opstk[sp].objval->type == CLOSURE) {
                Closure* close = opstk[sp--].objval->closure;
                callstk = new ActivationRecord(cpIdx, ip, callstk, close->env);
                for (int i = numArgs; i > 0; i--) {
                    callstk->locals[i] = opstk[sp--];
                }
                ip = close->func->start_ip;
            } else {
                cout <<"OH GOD OH GOD OH GOD"<<endl;
                running = false;
            }
        }
        void retProcedure() {
            ip = callstk->ret_addr;
            closeBlock();
            if (collector.ready()) {
                collector.run(callstk, globals, opstk, sp, &constPool);
            }
        }
        void instantiate(Instruction& inst) {
            ClassObject* master = constPool.get(inst.operand[0].intval).objval->object;
            ClassObject* clone = new ClassObject(master->name, master->scope);
            clone->instantiated = true;
            for (auto m : master->fields) {
                clone->fields[m.first] = StackItem();
            }
            opstk[++sp] = alloc.alloc(clone); 
        }
        void storeGlobal() {
            StackItem t = opstk[sp--];
            StackItem val = opstk[sp--];
            globals->locals[t.intval] =  val;
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
                cout<<"loaded Upval: "<<opstk[sp].toString()<<"from "<<inst.operand[0].intval<<" of scope "<<(fp)<<endl;
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
            if (top(1).type == OBJECT) {
                switch (top(1).objval->type) {
                    case LIST:
                        top(1) = (top(1).objval->list->at(top(0).numval)); sp--; 
                        return;
                    case STRING: 
                        char c = top(1).objval->strval->at(top(0).numval);
                        string str;
                        str.push_back(c);
                        top(1) = (alloc.alloc(new string(str))); sp--; 
                        return;
                }
            }
        }
        void storeIndexed(Instruction& inst) {
            if (top(1).type == OBJECT && top(1).objval->type == LIST) {
                top(1).objval->list->at(top(0).numval) = top(2); 
                sp--;
            }
        }
        void loadField(Instruction& inst) {
            if (top(0).type == OBJECT && top(0).objval->type == CLASS) {
                int idx = inst.operand[0].intval;    
                string fieldName = *(constPool.get(idx).objval->strval);
                auto object = top(0).objval->object;
                auto item = object->fields[fieldName];
                top(0) = item;
                return;
            }
        }
        void storeField(Instruction& inst) {
            if (top(0).objval->type == CLASS) {
                int idx = inst.operand[0].intval;    
                string fieldName = *(constPool.get(idx).objval->strval);
                top(0).objval->object->fields[fieldName] = top(1);
            }
        }
        void loadConst(Instruction& inst) {
            if (inst.operand[0].type == INTEGER) {
                opstk[++sp] = (constPool.get(inst.operand[0].intval));
            } else {
                opstk[++sp] = (inst.operand[0]);
            }
        }
        void loadAddress(Instruction& inst) {
            opstk[++sp] = (inst.operand[0]); 
        }
        void randNumber(Instruction& inst) {
            opstk[++sp] = fmod(rand(), constPool.get(inst.operand[0].intval).numval); 
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
        void appendList() {
            if (top(1).type == OBJECT && top(1).objval->type == LIST)
                top(1).objval->list->push_back(top(0));
            sp--;
        }
        void pushList() {
            if (top(1).type == OBJECT && top(1).objval->type == LIST)
                top(1).objval->list->push_front(top(0));
            sp-=2;
        }
        void listLength() {
            if (top().type == OBJECT && top().objval->type == LIST)
                top() = ((double)top().objval->list->size());
        }
        void makeRange() {
            double hi = opstk[sp--].numval;
            double lo = opstk[sp--].numval;
            if (hi < lo) swap(hi, lo);
            if (top(0).type != OBJECT || (top(0).objval->type != LIST)) {
                cout<<"Error: ranges need lists, yo."<<endl;
                return;
            }
            cout<<"Creating set of integers from "<<lo<<" to "<<hi<<endl;
            for (int i = lo; i <= hi; i++) {
                top(0).objval->list->push_back((double)i);
            }
        }
        void duplicateTop() {
            auto item = top(0);
            opstk[++sp] = item;
        }
        void haltvm() {
            running = false;
        }
        void printTopOfStack() {
            cout<<opstk[sp--].toString();
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
                    top(1).boolval = top(1).lessThan(top(0));
                } break;
                case VM_GT:   {
                    top(1).boolval = top(0).lessThan(top(1));
                } break;
                case VM_LTE: {
                top(1).boolval = (top(1).lessThan(top(0)) || (top(0).equals(top(1))));
                } break;
                case VM_GTE: {
                    top(1).boolval = (top(0).lessThan(top(1)) || (top(0).equals(top(1))));
                } break;
                case VM_EQU:  {
                    top(1).boolval = (top(0).equals(top(1))); break;
                } break;
                case VM_NEQ: {
                    top(1).boolval = !(top(0).equals(top(1))); break;
                } break;
                case VM_LOGIC_AND: {
                    top(1).boolval = (top(1).boolval && top(0).boolval);
                } break;
                case VM_LOGIC_OR: {
                    top(1).boolval = (top(1).boolval || top(0).boolval);
                } break;
            }
            top(1).type = BOOLEAN;
            sp--;
        }
        void arithmeticOperation(Instruction& inst) {
            switch (inst.operand[0].intval) {
                case VM_ADD:  {
                    top(1).add(top());
                } break;
                case VM_SUB:  {
                    top(1).sub(top());
                } break;
                case VM_MUL:  {
                    top(1).mul(top());
                } break;
                case VM_DIV:  {
                    top(1).div(top());
                } break;
                case VM_MOD:  {
                    top(1).mod(top());
                } break;
                default:
                    break;
            }
            sp--;
        }
        void execute(Instruction& inst) {
            switch (inst.op) {
                case list_append: { appendList(); } break;
                case list_push:   { pushList(); } break;
                case list_len: { listLength(); } break;
                case call:     { callProcedure(inst); } break;
                case retfun:   { retProcedure(); } break;
                case entblk:   { openBlock(inst); } break;
                case retblk:   { closeBlock(); } break;
                case jump:     { uncondBranch(inst); } break;
                case brf:      { branchOnFalse(inst); } break;
                case binop:    { binaryOperation(inst); } break;
                case unop:     { unaryOperation(inst); } break;
                case print:    { printTopOfStack(); } break;
                case newline:  { cout<<endl; } break;
                case halt:     { haltvm(); } break;
                case stglobal: { storeGlobal(); } break;
                case stupval:  { storeUpval(inst); } break;
                case stlocal:  { storeLocal(inst); } break;
                case stidx:    { storeIndexed(inst); } break;
                case stfield:  { storeField(inst); } break;
                case ldconst:  { loadConst(inst); } break;
                case ldglobal: { loadGlobal(inst); } break;
                case ldupval:  { loadUpval(inst); } break;
                case ldlocal:  { loadLocal(inst); } break;
                case ldfield:  { loadField(inst); } break;
                case ldidx:    { loadIndexed(inst); } break;
                case ldaddr:    { loadAddress(inst); } break;
                case mkclosure: { closeOver(inst); } break;
                case mkstruct:  { instantiate(inst); } break;
                case mkrange:   { makeRange(); }
                case ldrand:    { randNumber(inst); } break;
                case popstack:  { sp--; } break; 
                case incr:     { top(0).numval += 1; } break;
                case decr:     { top(0).numval -= 1; } break;
                default:
                    break;
            }       
        }
        Instruction& fetch() {
            return ip < codePage.size() && ip > -1 ? codePage[ip++]:haltSentinel;
        }
        void printInstruction(Instruction& inst) {
            cout<<"Instrctn: "<<ip<<": [0x0"<<inst.op<<"("<<instrStr[inst.op]<<"), "<<inst.operand[0].toString()<<","<<inst.operand[1].toString();
            if (inst.op == call) cout<<", "<<inst.operand[2].toString();
            cout<<"]  \n";
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
                    cout<<"\t   Lexical stack: \n";
                    int l = i;
                    auto t = x->access;
                    while (t != globals) {
                        cout<<"\t   \t    \t     "<<l++<<": [ ";
                        for (int j = 1; j <= 5; j++) {
                            cout<<(j)<<": "<<"{"<<t->locals[j].toString()<<"}, ";
                        }
                        cout<<"]"<<endl;
                        t = t->access;
                    }
                    cout<<"\t  \t   \t  ";
                    for (int j = 1; j <= 5; j++) {
                        cout<<(j)<<": "<<"{"<<t->locals[j].toString()<<"}, ";
                    }
                    cout<<endl;
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
            globals =  new ActivationRecord(GLOBAL_SCOPE,0, nullptr, nullptr);
            callstk = globals;
        }
        ~VM() {
            collector.run(callstk, globals, opstk, sp, &constPool);
            delete callstk;
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
                }
                if (verbosity > 0) cout<<"================"<<endl;
            }
        }
};



#endif