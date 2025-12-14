#ifndef callframe_hpp
#define callframe_hpp
#include <iostream>
#include "stackitem.hpp"
#include "instruction.hpp"
#include "gcobject.hpp"

static const int MAX_OP_STACK = 1255;
static const int MAX_LOCAL = 255;

struct ActivationRecord : GCObject {
    int cp_index;
    int num_args;
    int num_locals;
    int ret_addr;
    int refCount;
    StackItem locals[MAX_LOCAL];
    ActivationRecord* control;
    ActivationRecord* access;
    ActivationRecord(int idx = -1, int ra = 0, int args = 0, ActivationRecord* calling = nullptr, ActivationRecord* defining = nullptr) {
        cp_index = idx;
        ret_addr = ra;
        num_args = args;
        control = calling;
        access = defining;
        refCount = 1;
        if (access) access->refCount += 1;
    }
    ~ActivationRecord() {
        if (access && --access->refCount == 0) {
            delete access;
        }
    }
};

struct Closure : GCObject {
    Function* func;
    ActivationRecord* env;
    Closure(Function* f, ActivationRecord* e) : func(f), env(e) { env->refCount += 1; }
    Closure(Function* f) : func(f), env(nullptr) { }
    Closure(const Closure& c) {
        func = c.func;
        env = c.env;
    }
    ~Closure() {
        if (env && --env->refCount == 0) {
            delete env;
        }
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
#endif