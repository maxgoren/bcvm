#ifndef stackitem_hpp
#define stackitem_hpp
#include <iostream>
#include <unordered_map>
#include <cmath>
#include <deque>
#include "alloc.hpp"
#include "heapitem.hpp"
using namespace std;


enum SIType {
    NIL, INTEGER, NUMBER, BOOLEAN, OBJECT
 };

struct StackItem {
    int type;
    union {
        int intval;
        bool boolval;
        double numval;
        GCItem* objval;
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
            case OBJECT: return objval->toString();
            case BOOLEAN: return boolval ? "true":"false";
            case NIL: return "(nil)";
            
        }
        return "(nil)";
    }
    StackItem(int value) { intval = value; type = INTEGER; }
    StackItem(double value) { numval = value; type = NUMBER; }
    StackItem(bool balue) { boolval = balue; type = BOOLEAN; }
    StackItem(string value) { objval = alloc.alloc(new string(value)); type = OBJECT; }
    StackItem(Closure* c) { objval = alloc.alloc(c); type = OBJECT; }
    StackItem(deque<StackItem>* l) { objval = alloc.alloc(l); type = OBJECT; }
    StackItem(ClassObject* o) { objval = alloc.alloc(o); type = OBJECT; }
    StackItem(GCItem* i) { objval = i; type = OBJECT; }
    StackItem() { type = NIL; intval = -66; }
    StackItem(const StackItem& si) {
        type = si.type;
        switch (si.type) {
            case INTEGER: intval = si.intval; break;
            case NUMBER: numval = si.numval; break;
            case BOOLEAN: boolval = si.boolval; break;
            case OBJECT: objval = si.objval; break;
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
            case OBJECT: objval = si.objval; break;
        }
        return *this;
    }
    bool lessThan(StackItem& si) {
        switch (si.type) {
            case INTEGER: {
                switch (type) {
                    case INTEGER: return intval < si.intval;
                    case NUMBER: return  numval < si.intval;
                    case BOOLEAN: return boolval < si.intval;
                    case OBJECT: return 1;
                }
            } break;
            case BOOLEAN: {
                switch (type) {
                    case INTEGER: return intval < si.boolval;
                    case NUMBER: return  numval < si.boolval;
                    case BOOLEAN: return boolval < si.boolval;
                    case OBJECT: 1;
                }
            } break;
            case NUMBER: {
                switch (type) {
                    case INTEGER: return intval < si.numval;
                    case NUMBER:  return  numval < si.numval;
                    case BOOLEAN: return boolval < si.numval;
                    case OBJECT: 1;
                }
            } break;
            case OBJECT: {
            switch (type) {
                    case INTEGER: return false;
                    case NUMBER: return  false;
                    case BOOLEAN: return false;
                    case OBJECT: {
                        if (objval->type == STRING && si.objval->type == STRING)
                            return strcmp(objval->strval->data(), si.objval->strval->data()) < 0;
                    }
                }
            } break;
        }
        return false;
    }
    bool equals(StackItem& rhs) {
        if (type != rhs.type)
            return false;
        switch (type) {
            case INTEGER: return intval == rhs.intval;
            case NUMBER:  return numval == rhs.numval;
            case BOOLEAN: return boolval == rhs.boolval;
            case NIL:     return true;
            case OBJECT:  return objval->equals(rhs.objval);
        }
        return false;
    }
    StackItem& add(StackItem& rhs) {
        if (type == OBJECT || rhs.type == OBJECT) {
            string str;
            for (char c : toString()) {
                str.push_back(c);
            }
            for (char c : rhs.toString()) {
                str.push_back(c);
            }
            objval = alloc.alloc(new string(str));
            type = OBJECT;
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
        } else if (type == STRING && rhs.type == INTEGER) {
            string* ns = new string(*objval->strval);
            for (int i = 1; i < rhs.numval; i++) {
                *ns += (*objval->strval);
            }
            objval->strval = ns;
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

struct ClassObject  {
    string name;
    int cpIdx;
    unordered_map<string, StackItem> fields;
    bool instantiated;
    Scope* scope;
    ClassObject(string n = "",  Scope* s = nullptr) {
        name = n;
        scope = s;
    }
};

string classToString(ClassObject* obj) {
    if (obj == nullptr) {
        return "nil";
    }
    if (obj->instantiated) {
        string str = obj->name + "{ ";
        for (auto m : obj->fields) {
            str += m.first + ": " + m.second.toString() +" ";
        }
        str += "} ";
        return str;
    }
    return obj->name;
}

void freeClass(ClassObject* obj) {
    if (obj != nullptr) {
        delete obj;
    }
}

string listToString(deque<StackItem>* list) {
        string str = "[";
        for (auto m : *list) {
            str += m.toString() + " ";
        }
        return str + "]";
}


#endif