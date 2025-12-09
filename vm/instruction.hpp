#ifndef instruction_hpp
#define instruction_hpp
#include "stackitem.hpp"

enum VMInstruction {
    defun, label, 
    ldfield, ldconstpl,
    ldconst, ldglobal, 
    ldlocal,ldupval,
    ldlocaladdr, ldglobaladdr, 
    stglobal, stlocal, 
    stupval, stfield,
    call, retfun, 
    entblk, retblk,
    jump, brf, incr, decr, dup,
    binop, unop, mkclosure, 
    defstruct, mkstruct, popstack,
    list_append, list_push, list_len,
    print, newline, halt
};

string instrStr[] = { "defun", "label", "ldfield", "ldconstpl", "ldconst", "ldglobal", "ldlocal", "ldupval", "ldlocaladdr", "ldglobaladdr",
                     "stglobal", "stlocal", "stupval", "stfield", "call", "retfun", "entblk", "retblk", "jump", "brf", "incr", "decr", "dup", 
                     "binop", "unop", "mkclosure", "defstruct", "mkstruct", "popstack", "append", "push", "list_len", "print", "newline", "halt"};

enum VMOperators {
    VM_ADD = 1, VM_SUB = 2, VM_MUL = 3, VM_DIV = 4, 
    VM_MOD=5, VM_LT=7, VM_GT=8, VM_EQU=9, VM_NEQ=10,
    VM_NEG = 11, VM_LTE = 12, VM_GTE = 13, VM_LOGIC_AND = 14,
    VM_LOGIC_OR = 15
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