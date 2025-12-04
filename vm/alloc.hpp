#ifndef gc_hpp
#define gc_hpp
#include <iostream>
#include <unordered_set>
#include <list>
#include <deque>
using namespace std;

enum GCType {
    STRING, FUNCTION, CLOSURE, LIST
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

string listToString(deque<StackItem>* list);

struct GCItem {
    GCType type;
    bool marked;
    union {
        string* strval;
        Function* func;
        Closure* closure;
        deque<StackItem>* list;
    };
    GCItem(string* s) : type(STRING), strval(s) { }
    GCItem(Function* f) : type(FUNCTION), func(f) { }
    GCItem(Closure* c) : type(CLOSURE), closure(c) { }
    GCItem(deque<StackItem>* l) : type(LIST), list(l) { }
    GCItem(const GCItem& si) {
        switch (si.type) {
            case STRING: strval = si.strval; break;
            case FUNCTION: func = si.func; break;
            case CLOSURE: closure = si.closure; break;
            case LIST: list = si.list; break;
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
        }
        return "(nil)";
    }
};

class GCAllocator {
    private:
        friend class GarbageCollector;
        unordered_set<GCItem*> live_items;
        list<GCItem*> free_list;
    public:
        GCAllocator() {

        }
        GCItem* alloc(string* s) {
            GCItem* x;
            if (!free_list.empty()) {
                x = free_list.back(); 
                free_list.pop_back();
                x->type = STRING;
                x->strval = s;
            } else {
                x = new GCItem(s);
            }
            live_items.insert(x);
            return x;
        }
        GCItem* alloc(Closure* c) {
            GCItem* x;
            if (!free_list.empty()) {
                x = free_list.back(); 
                free_list.pop_back();
                x->type = CLOSURE;
                x->closure = c;
            } else {
                x = new GCItem(c);
            }
            live_items.insert(x);
            return x;
        }
        GCItem* alloc(Function* f) {
            GCItem* x;
            if (!free_list.empty()) {
                x = free_list.back(); 
                free_list.pop_back();
                x->type = FUNCTION;
                x->func = f;
            } else {
                x = new GCItem(f);
            }
            live_items.insert(x);
            return x;
        }
        GCItem* alloc(deque<StackItem>* l) {
            GCItem* x;
            if (!free_list.empty()) {
                x = free_list.back(); 
                free_list.pop_back();
                x->type = LIST;
                x->list = l;
            } else {
                x = new GCItem(l);
            }
            live_items.insert(x);
            return x;
        }
        unordered_set<GCItem*>& getLiveList() {
            return live_items;
        }
        list<GCItem*> getFreeList() {
            return free_list;
        }
};


#endif