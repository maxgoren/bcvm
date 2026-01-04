#ifndef closure_hpp
#define closure_hpp
#include "callframe.hpp"
using namespace std;

struct BlockScope;

struct Function {
    string name;
    int start_ip;
    BlockScope* scope;
    Function(string n, int sip, BlockScope* sc) : name(n), start_ip(sip), scope(sc) { }
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
    ActivationRecord* env;
    Closure(Function* f, ActivationRecord* e) : func(f), env(e) { }
    Closure(Function* f) : func(f), env(nullptr) { }
    Closure(const Closure& c) {
        func = c.func;
        env = c.env;
    }
    ~Closure() {

    }
    Closure& operator=(const Closure& c) {
        if (this != &c) {
            func = c.func;
            env = c.env;
        }
        return *this;
    }
};

string closureToString(Closure* closure) {
    return "(closure)" + closure->func->name + ", " + to_string(closure->func->start_ip);
}

void freeClosure(Closure* cl) {
    if (cl != nullptr) {
        delete cl;
    }   
}

void freeFunction(Function* f) {
    if (f != nullptr) {
        delete f;
    }
}

#endif