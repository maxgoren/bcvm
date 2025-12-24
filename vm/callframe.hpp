#ifndef callframe_hpp
#define callframe_hpp
#include <iostream>
#include "stackitem.hpp"
#include "instruction.hpp"
#include "gcobject.hpp"

static const int MAX_LOCAL = 255;

struct ActivationRecord : GCObject {
    int cp_index;
    int ret_addr;
    StackItem locals[MAX_LOCAL];
    ActivationRecord* control;
    ActivationRecord* access;
    ActivationRecord(int idx = -1, int ra = 0, ActivationRecord* calling = nullptr, ActivationRecord* defining = nullptr) {
        cp_index = idx;
        ret_addr = ra;
        control = calling;
        access = defining;
        isAR = true;
        alloc.registerObject(this);
    }
    ActivationRecord(const ActivationRecord& ar) {
        cp_index = ar.cp_index;
        ret_addr = ar.ret_addr;
        control = ar.control;
        access = ar.access;
        isAR = true;
        marked = ar.marked;
    }
    ~ActivationRecord() {
        
    }
};

void freeAR(ActivationRecord* to) {
    if (to != nullptr) {
        delete to;
    }
}


#endif