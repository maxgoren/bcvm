#ifndef heapitem_hpp
#define heapitem_hpp
#include <iostream>
#include <unordered_set>
#include <list>
#include <deque>
#include "gcobject.hpp"
using namespace std;

enum GCType {
    STRING, FUNCTION, CLOSURE, LIST, CLASS, NILPTR
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

Function* makeFunction(string name, int start, Scope* s) {
    return new Function(name, start, s); 
}

struct StackItem;
struct ClassObject;
struct Closure;

string closureToString(Closure* cl);
string listToString(deque<StackItem>* list);
string classToString(ClassObject* obj);

struct GCItem : GCObject {
    GCType type;
    union {
        string* strval;
        Function* func;
        Closure* closure;
        deque<StackItem>* list;
        ClassObject* object;
    };
    GCItem(string* s) : type(STRING), strval(s) { }
    GCItem(Function* f) : type(FUNCTION), func(f) { }
    GCItem(Closure* c) : type(CLOSURE), closure(c) { }
    GCItem(deque<StackItem>* l) : type(LIST), list(l) { }
    GCItem(ClassObject* o) : type(CLASS), object(o) { } 
    GCItem() : type(NILPTR) { }
    GCItem(const GCItem& si) {
        switch (si.type) {
            case STRING: strval = si.strval; break;
            case FUNCTION: func = si.func; break;
            case CLOSURE: closure = si.closure; break;
            case LIST: list = si.list; break;
            case CLASS: object = si.object; break;
        }
        type = si.type;
    }
    GCItem& operator=(const GCItem& si) {
        if (this != &si) {
            switch (si.type) {
                case STRING: strval = si.strval; break;
                case FUNCTION: func = si.func; break;
                case CLOSURE: closure = si.closure; break;
                case LIST: list = si.list; break;
                case CLASS: object = si.object; break;
            }
            type = si.type;
        }
        return *this;
    }
    string toString() {
        switch (type) {
            case STRING: return *(strval);
            case FUNCTION: return "(func)" + func->name;
            case CLOSURE: return closureToString(closure);
            case LIST: return listToString(list);
            case CLASS: return "(class)" + classToString(object);
        }
        return "(nil)";
    }
    bool equals(GCItem* rhs) {
        if (type != rhs->type)
            return false;
        switch (type) {
            case STRING: return *strval == *rhs->strval;
            case FUNCTION: return func->name == rhs->func->name;
            case LIST: return listToString(list) == listToString(rhs->list);
            case CLASS: return object == rhs->object;
            case CLOSURE: return closure == rhs->closure;
        }
        return false;
    }
};

#endif