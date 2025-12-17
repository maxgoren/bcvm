#ifndef closure_hpp
#define closure_hpp
#include "callframe.hpp"

struct Closure : GCObject {
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

#endif