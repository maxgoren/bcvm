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
        alloc.getLiveList().insert(this);
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