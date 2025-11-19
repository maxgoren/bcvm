#ifndef stackitem_hpp
#define stackitem_hpp
#include <iostream>
#include <cmath>
#include <vector>
using namespace std;

enum VMInstruction {
    defun, label, ldfield,
    ldconst, ldglobal, ldlocal,
    ldlocaladdr, ldglobaladdr, 
    stglobal, stlocal, stfield,
    call, entfun, retfun,
    jump, brf, //juump, branch on false
    binop, unop, //binary oper, unary oper
    append, push, first, rest,
    print, halt
};

string instrStr[] = { ".def", "label", "ldfield", "ldconst", "ldglobal", "ldlocal", "ldlocaladdr", "ldglobaladdr",
                     "stglobal", "stlocal", "stfield", "call", "entfun", "retfun", "jump", "brf", "binop", "unop", 
                     "append", "push", "first", "rest", "print", "halt"};

enum VMOperators {
    VM_ADD = 1, VM_SUB = 2, VM_MUL = 3, VM_DIV = 4, 
    VM_MOD=5, VM_LT=7, VM_GT=8, VM_EQU=9, VM_NEQ=10,
    VM_NEG = 11
};


enum SIType {
   NIL, INTEGER, NUMBER, STRING, BOOLEAN, FUNCTION, CLOSURE, LIST
};

struct Scope;

struct Function {
    string name;
    int start_ip;
    Scope* scope;
    Function(string n, int sip, Scope* sc) : name(n), start_ip(sip), scope(sc) { }
    Function(const Function& f) {
        name = f.name;
        start_ip  = f.start_ip;
        scope = f.scope;
    }
    Function& operator=(const Function& f) {
        if (this != &f) {
            name = f.name;
            start_ip  = f.start_ip;
            scope = f.scope;
        }
        return *this;
    }
};

struct Closure {
    Function* func;
    Closure(Function* f) : func(f) { }
};

struct StackItem {
    int type;
    union {
        int intval;
        bool boolval;
        double numval;
        string* strval;
        Function* func;
        Closure* closure;
        vector<StackItem>* list;
    };
    string toString() {
        switch (type) {
            case INTEGER: {
                return to_string(intval);
            } break;
            case NUMBER: {
                if (fmod(numval,1) == 0)
                    return to_string((int)numval);
                return to_string(numval);
            } break;
            case STRING: return *strval;
            case BOOLEAN: return boolval ? "true":"false";
            case NIL: return "(nil)";
            case FUNCTION: return "(func)" + func->name;
            case CLOSURE: return "(closure)" + closure->func->name + ", " + to_string(closure->func->start_ip);
            case LIST: {
                string str = "[";
                for (auto m : *list) {
                    str += m.toString() + " ";
                }
                return str + "]";
            };
        }
        return "(nil)";
    }
    StackItem(int value) { intval = value; type = INTEGER; }
    StackItem(double value) { numval = value; type = NUMBER; }
    StackItem(string value) { strval = new string(value); type = STRING; }
    StackItem(bool balue) { boolval = balue; type = BOOLEAN; }
    StackItem(Function* f) { func = f; type = FUNCTION; }
    StackItem(Closure* c) { closure = c; type = CLOSURE; }
    StackItem(vector<StackItem>* l) { list = l; type = LIST; }
    StackItem() { type = NIL; intval = -66; }
    StackItem(const StackItem& si) {
        type = si.type;
        switch (si.type) {
            case INTEGER: intval = si.intval; break;
            case NUMBER: numval = si.numval; break;
            case BOOLEAN: boolval = si.boolval; break;
            case STRING: strval = si.strval; break;
            case FUNCTION: func = si.func; break;
            case CLOSURE: closure = si.closure; break;
            case LIST: list = si.list; break;
        }
    }
    StackItem& operator=(const StackItem& si) {
        if (this == &si)
            return *this;
        type = si.type;
        switch (si.type) {
            case INTEGER: intval = si.intval; break;
            case NUMBER: numval = si.numval; break;
            case BOOLEAN: boolval = si.boolval; break;
            case STRING: strval = si.strval; break;
            case FUNCTION: func = si.func; break;
            case CLOSURE: closure = si.closure; break;
            case LIST: list = si.list; break;
        }
        return *this;
    }
    bool compareTo(StackItem& si) {
        switch (si.type) {
            case INTEGER: {
                switch (type) {
                    case INTEGER: return intval < si.intval;
                    case NUMBER: return  numval < si.intval;
                    case BOOLEAN: return boolval < si.intval;
                    case STRING: return 1;
                }
            } break;
            case BOOLEAN: {
                switch (type) {
                    case INTEGER: return intval < si.boolval;
                    case NUMBER: return  numval < si.boolval;
                    case BOOLEAN: return boolval < si.boolval;
                    case STRING: 1;
                }
            } break;
            case NUMBER: {
                switch (type) {
                    case INTEGER: return intval < si.numval;
                    case NUMBER:  return  numval < si.numval;
                    case BOOLEAN: return boolval < si.numval;
                    case STRING: 1;
                }
            } break;
            case STRING: {
            switch (type) {
                    case INTEGER: return -1;
                    case NUMBER: return  -1;
                    case BOOLEAN: return -1;
                    case STRING: return strcmp(strval->data(), si.strval->data()) < 0;
                }
            } break;
        }
        return false;
    }
    StackItem& add(StackItem& rhs) {
        if (rhs.type == STRING) {
            type = STRING;
            string str = toString() + rhs.toString();
            strval = new string(str);
        } else {
            double v = rhs.type == INTEGER ? rhs.intval:rhs.numval;
            switch (type) {
                case INTEGER: {
                    intval += v;
                } break;
                case NUMBER: {
                    numval += v;
                } break;
                case BOOLEAN: {
                    boolval += v;
                } break;
                case STRING: {
                    strval->append(rhs.toString());
                } break;
            }
        }
        return *this;
    }
    StackItem& sub(StackItem& rhs) {
        if (rhs.type == STRING) {
            return *this;
        } else {
            double v = rhs.type == INTEGER ? rhs.intval:rhs.numval;
            switch (type) {
                case INTEGER: {
                    intval -= v;
                } break;
                case NUMBER: {
                    numval -= v;
                } break;
                case BOOLEAN: {
                    boolval -= v;
                } break;
            }
        }
        return *this;
    }
    StackItem& mul(StackItem& rhs) {
        if (rhs.type == STRING) {
            return *this;
        } else {
            double v = rhs.type == INTEGER ? rhs.intval:rhs.numval;
            switch (type) {
                case INTEGER: {
                    intval *= v;
                } break;
                case NUMBER: {
                    numval *= v;
                } break;
                case BOOLEAN: {
                    boolval *= v;
                } break;
            }
        }
        return *this;
    }
    StackItem& div(StackItem& rhs) {
        if (rhs.type == STRING) {
            return *this;
        } else {
            double v = rhs.type == INTEGER ? rhs.intval:rhs.numval;
            switch (type) {
                case INTEGER: {
                    intval /= v;
                } break;
                case NUMBER: {
                    numval /= v;
                } break;
                case BOOLEAN: {
                    boolval /= v;
                } break;
            }
        }
        return *this;
    }
    StackItem& mod(StackItem& rhs) {
        if (rhs.type == STRING) {
            return *this;
        } else {
           numval = fmod(numval,rhs.numval); 
        }
        return *this;
    }
};

struct Instruction {
    VMInstruction op;
    StackItem operand[3];
    Instruction(VMInstruction instr, StackItem val, StackItem val2, StackItem val3) : op(instr) { operand[0] = val; operand[1] = val2; operand[2] = val3; } 
    Instruction(VMInstruction instr, StackItem val, StackItem val2) : op(instr) { operand[0] = val; operand[1] = val2; }
    Instruction(VMInstruction instr, StackItem val) : op(instr) { operand[0] = val; }
    Instruction(VMInstruction instr = halt) : op(instr) { }
};

#endif