#ifndef vm_hpp
#define vm_hpp
#include <vector>
#include <memory>
#include "stackitem.hpp"
#include "instruction.hpp"
using namespace std;

static const int MAX_OP_STACK = 1255;
static const int MAX_CALL_STACK = 1255;

struct ActivationRecord {
    int cpIdx;
    int num_args;
    int num_locals;
    int returnAddress;
    StackItem locals[255];
    shared_ptr<ActivationRecord> control;
    shared_ptr<ActivationRecord> access;
    ActivationRecord(int n = -1, int ra = 0, int numArgs = 0, shared_ptr<ActivationRecord> calling = nullptr, shared_ptr<ActivationRecord> defining = nullptr) {
        cpIdx = n;
        returnAddress = ra;
        num_args = numArgs;
        control = calling;
        access = defining;
    }
};

class GarbageCollector {
    private:
        int GC_LIMIT;
        void markObject(GCItem* object) {
            if (object == nullptr)
                return;
            vector<GCItem*> sf;
            sf.push_back(object);
            while (!sf.empty()) {
                GCItem* curr = sf.back(); sf.pop_back();
                if (curr != nullptr && curr->marked == false) {
                    curr->marked = true;
                    switch (curr->type) {
                        case STRING: { cout<<"Marked string: ["<<*curr->strval<<"]"<<endl; } break;
                        case FUNCTION: { cout<<"Marked function: ["<<curr->func->name<<"]"<<endl; } break;
                        case CLOSURE: {
                            cout<<"Marked closure"<<endl;
                            int numLocals = curr->closure->func->scope->symTable.size();
                            for (int i = 0; i < 255; i++) {
                                if (curr->closure->env->locals[i].type == OBJECT) {
                                    sf.push_back(curr->closure->env->locals[i].objval);
                                }
                            }
                        } break;
                        case LIST: {      
                            cout<<"Marked list: "<<curr->toString()<<endl;
                            for (auto & it : *curr->list) {
                                if (it.type == OBJECT && it.objval != nullptr) {
                                    sf.push_back(it.objval);
                                }
                            }
                        } break;
                        default:
                            break;
                    }
                }   
            }
        }
        void markItem(StackItem& item) {
            if (item.type == OBJECT) {
                markObject(item.objval);
            }
        }
    public:
        GarbageCollector() {
            GC_LIMIT = 50;
        }
        bool ready() {
            return gc.getLiveList().size() == GC_LIMIT;
        }
        void mark(ActivationRecord* callstk, ConstPool& constPool) {
            cout<<"start mark phase"<<endl;
            for (auto & it = callstk; it != nullptr; it = it->control.get()) {
                for (int i = 0; i < 255; i++) {
                    markItem(it->locals[i]);
                }
            }
            for (int i = 0; i < constPool.size(); i++) {
                markItem(constPool.get(i));
            }
            cout<<"Marking complete."<<endl;
        }
        void sweep() {
            unordered_set<GCItem*> nextGen;
            unordered_set<GCItem*> toFree;
            for (auto & it : gc.getLiveList()) {
                if (it->marked == true) {
                    nextGen.insert(it);
                } else {
                    toFree.insert(it);
                }
            }
            gc.getLiveList().swap(nextGen);
            cout<<"Collecting: "<<toFree.size()<<", lives: "<<gc.getLiveList().size()<<","<<nextGen.size()<<endl;
            for (auto it : toFree) {
                gc.getFreeList().push_back(it);
            }
            GC_LIMIT *= 2;
        }
        void run(ActivationRecord* callstk, ConstPool& constPool) {
            mark(callstk, constPool);
            sweep();
        }
};

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
        shared_ptr<ActivationRecord> callstk;
        shared_ptr<ActivationRecord> globals;
        StackItem opstk[MAX_OP_STACK];
        ActivationRecord* walkChain(int d) {
            if (d == GLOBAL_SCOPE) return globals.get();
            if (d == LOCAL_SCOPE) return callstk.get();
            auto x = callstk.get();
            while (x != nullptr && d > 0) {
                x = x->access.get();
                d--;
              //  cout<<".";
            }
            return x;
        }
        StackItem& top(int depth = 0) {
            return opstk[sp-depth];
        }
        void closeOver(Instruction& inst) {
            int func_id = inst.operand[0].intval;
            int depth = inst.operand[1].intval;
            //First run through, we stash current activation record as closures defining env
            //Subsequent invocations find the most recently created activation record for that function
            ActivationRecord* closeOver = callstk.get();
            string fn_name = constPool.get(func_id).objval->closure->func->name;
            if (constPool.get(func_id).objval->closure->env == nullptr) {
                constPool.get(func_id).objval->closure->env = callstk; 
            } else {
                auto x = callstk.get();
                while (x != nullptr && x != globals.get()) {
                    if (x->cpIdx == func_id) {
                        closeOver = x;
                        break;
                    }
                    x = x->control.get();
                }
                if (x == nullptr || x == globals.get())
                    closeOver = callstk.get();
                else closeOver = x;
            }
            opstk[++sp] = StackItem(gc.alloc(new Closure(constPool.get(func_id).objval->closure->func, closeOver)));
        }
        void openBlock(Instruction& inst) {
            callstk = make_shared<ActivationRecord>(BLOCK_CPIDX, ip, inst.operand[1].intval, callstk, callstk);
        }
        void closeBlock() {
            if (callstk != nullptr && callstk->control != nullptr)
                callstk = callstk->control;
            if (verbLev > 1)
                cout<<"Leaving scope."<<endl;
        }
        Closure* getProcedureForCall(int cpIdx) {
            if (cpIdx == -1)
                return opstk[sp--].objval->closure;
            if (opstk[sp].type == OBJECT && opstk[sp].objval->type == CLOSURE)
                sp--;
            return constPool.get(cpIdx).objval->closure;
        }
        void callProcedure(Instruction& inst) {
            int returnAddr = ip;
            int numArgs = inst.operand[1].intval;
            int lexDepth = inst.operand[2].intval;
            int cpIdx = inst.operand[0].intval;
            Closure* close = getProcedureForCall(cpIdx);
            callstk = make_shared<ActivationRecord>(cpIdx, returnAddr, numArgs, callstk, close->env);            
            for (int i = numArgs; i > 0; i--) {
                callstk->locals[i] = opstk[sp--];
            }
            callstk->returnAddress = returnAddr;
            ip = close->func->start_ip;
        }
        void retProc() {
            ip = callstk->returnAddress;
            closeBlock();
            collector.run(callstk.get(), constPool);
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
            top(1) = (top(1).objval->list->at(top(0).numval));
            sp--;
        }
        void indexed_store(Instruction& inst) {
            top(1).objval->list->at(top(0).numval) = top(2);
            top(2) = top(1);
            top(1) = top(0);
            sp--;
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
        void appendList() {
            StackItem item = opstk[sp--];
            if (top().type == OBJECT && top().objval->type == LIST)
                top().objval->list->push_back(item);
        }
        void pushList() {
            StackItem item = opstk[sp--];
            if (top().type == OBJECT && top().objval->type == LIST)
                top().objval->list->push_front(item);
            sp--;
        }
        void listLength() {
            if (top().type == OBJECT && top().objval->type == LIST)
                opstk[++sp] = ((double)opstk[sp--].objval->list->size());
        }
        void haltvm() {
            running = false;
        }
        void printTopOfStack() {
            cout<<opstk[sp--].toString()<<endl;
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
                top(1).boolval = (top(1).lessThan(top(0)) || (!top(1).lessThan(top(0)) && !top(0).lessThan(top(1))));
                } break;
                case VM_GTE: {
                    top(1).boolval = (top(0).lessThan(top(1)) || (!top(1).lessThan(top(0)) && !top(0).lessThan(top(1))));
                } break;
                case VM_EQU:  {
                    top(1).boolval = (!top(1).lessThan(top(0)) && !top(0).lessThan(top(1)));
                } break;
                case VM_NEQ: {
                    top(1).boolval = (top(1).lessThan(top(0)) || top(0).lessThan(top(1)));
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
                case list_append: { appendList();   } break;
                case list_push:  { pushList(); } break;
                case list_len:  { listLength(); } break;
                case call:     { callProcedure(inst); } break;
                case retfun:   { retProc(); } break;
                case entblk:   { openBlock(inst); } break;
                case retblk:   { closeBlock(); } break;
                case incr:      { top(0).numval += 1; } break;
                case decr:      { top(0).numval -= 1; } break;
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
                case popstack:  { sp--; } break; 
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
            while (x != x->control->control) {
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
            globals =  make_shared<ActivationRecord>(GLOBAL_SCOPE,0, 0, nullptr, nullptr);
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
            if (collector.ready()) {
                collector.run(callstk.get(), constPool);
            }
        }
};



#endif