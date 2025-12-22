#ifndef heapitem_hpp
#define heapitem_hpp
#include <iostream>
#include <unordered_set>
#include <list>
#include <deque>
#include "gcobject.hpp"
using namespace std;

enum GCType {
    STRING, FUNCTION, CLOSURE, LIST, CLASS, REF, NILPTR
};

struct Scope;
struct Function;
struct StackItem;
struct ClassObject;
struct Closure;

string closureToString(Closure* cl);
string listToString(deque<StackItem>* list);
string classToString(ClassObject* obj);

void freeClosure(Closure* cl);
void freeClass(ClassObject* obj);

struct GCItem : GCObject {
    GCType type;
    union {
        string* strval;
        Function* func;
        Closure* closure;
        deque<StackItem>* list;
        ClassObject* object;
        StackItem* reference;
    };
    GCItem(string* s) : type(STRING), strval(s) { }
    GCItem(Function* f) : type(FUNCTION), func(f) { }
    GCItem(Closure* c) : type(CLOSURE), closure(c) { }
    GCItem(deque<StackItem>* l) : type(LIST), list(l) { }
    GCItem(ClassObject* o) : type(CLASS), object(o) { } 
    GCItem(StackItem* r) : type(REF), reference(r) { }
    GCItem() : type(NILPTR) { }
    GCItem(const GCItem& si) {
        switch (si.type) {
            case STRING: strval = si.strval; break;
            case FUNCTION: func = si.func; break;
            case CLOSURE: closure = si.closure; break;
            case LIST: list = si.list; break;
            case CLASS: object = si.object; break;
            case REF: reference = si.reference; break;
        }
        type = si.type;
        isAR = si.isAR;
        marked = si.marked;
    }
    GCItem& operator=(const GCItem& si) {
        if (this != &si) {
            switch (si.type) {
                case STRING: strval = si.strval; break;
                case FUNCTION: func = si.func; break;
                case CLOSURE: closure = si.closure; break;
                case LIST: list = si.list; break;
                case CLASS: object = si.object; break;
                case REF: reference = si.reference; break;
            }
            type = si.type;
            isAR = si.isAR;
            marked = si.marked;
        }
        return *this;
    }
    string toString() {
        switch (type) {
            case STRING: return *(strval);
            case FUNCTION: return "(func)";
            case CLOSURE: return closureToString(closure);
            case LIST: return listToString(list);
            case CLASS: return "(class)" + classToString(object);
            case REF: return "(reference)";
        }
        return "(nil)";
    }
    bool equals(GCItem* rhs) {
        if (type != rhs->type)
            return false;
        switch (type) {
            case STRING: return *strval == *rhs->strval;
            case FUNCTION: return func == rhs->func;
            case LIST: return listToString(list) == listToString(rhs->list);
            case CLASS: return object == rhs->object;
            case CLOSURE: return closure == rhs->closure;
            case REF:   return false;
        }
        return false;
    }
};

#endif