#ifndef heapitem_hpp
#define heapitem_hpp
#include <iostream>
#include <unordered_set>
#include <list>
#include <deque>
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

struct ActivationRecord;

struct Closure {
    Function* func;
    ActivationRecord* env;
    Closure(Function* f, ActivationRecord* e) : func(f), env(e) { }
    Closure(Function* f) : func(f), env(nullptr) { }
    Closure(const Closure& c) {
        func = c.func;
        env = c.env;
    }
    Closure& operator=(const Closure& c) {
        if (this != &c) {
            func = c.func;
            env = c.env;
        }
        return *this;
    }
};

struct StackItem;
struct ClassObject;

string listToString(deque<StackItem>* list);
string classToString(ClassObject* obj);

struct GCItem {
    GCType type;
    bool marked;
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
            case CLASS: object = si.object;
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
                case CLASS: object = si.object;
            }
            type = si.type;
        }
        return *this;
    }
    string toString() {
        switch (type) {
            case STRING: return *(strval);
            case FUNCTION: return "(func)" + func->name;
            case CLOSURE: return "(closure)" + closure->func->name + ", " + to_string(closure->func->start_ip);
            case LIST: return listToString(list);
            case CLASS: return classToString(object);
        }
        return "(nil)";
    }
};

#endif