#ifndef gc_hpp
#define gc_hpp
#include <vector>
#include <memory>
#include "constpool.hpp"
#include "stackitem.hpp"
#include "instruction.hpp"
using namespace std;

class GarbageCollector {
    private:
        void markObject(GCItem*& curr) {
            if (curr == nullptr)
                return;
            if (curr != nullptr && curr->marked == false) {
                curr->marked = true;
                if (curr->type == LIST && curr->list != nullptr) {
                    for (auto & it : *curr->list) {
                        markItem(&it);
                    }
                } else if (curr->type == CLASS && curr->object != nullptr) {
                    for (auto & it : curr->object->fields) {
                        markItem(&it.second);
                    }
                } else if (curr->type == CLOSURE && curr->closure != nullptr) {
                    markAR(curr->closure->env);
                }
            }
        }
        void markItem(StackItem* si) {
            if (si->type == OBJECT) {
                markObject(si->objval);
            }
        }
        void markAR(ActivationRecord* callframe) {
            ActivationRecord* ar = callframe;
            if (ar != nullptr && !ar->marked) {
                ar->marked = true;
                for (int i = 0; i < 255; i++) {
                    markItem(&ar->locals[i]);
                }
                markAR(ar->access);
                markAR(ar->control);
            }
        }
        void sweep() {
            unordered_set<GCObject*> nextGen;
            int far = 0, fri = 0, ltn = 0;
            for (auto & it : alloc.getLiveList()) {
                if (it->marked) {
                    it->marked = false;
                    nextGen.insert(it);
                } else {
                    if (it->isAR) {
                        freeAR((ActivationRecord*)it);
                    } else {
                        alloc.free((GCItem*)it);
                    }
                }
            }
            alloc.getLiveList().swap(nextGen);
        }
        void markOpStack(StackItem ops[], int sp) {
            for (int i = sp; i >= 0; i--) {
                markItem(&ops[i]);
            }
            int i = 1;
        }
        void markConstPool(ConstPool* constPool) {
            for (int i = 0; i < constPool->maxN; i++) {
                if (constPool->data[i].type == OBJECT) { 
                    constPool->data[i].objval->marked = true;
                    if (constPool->data[i].objval->type == CLOSURE && constPool->data[i].objval->closure->env != nullptr) {
                        markAR(constPool->data[i].objval->closure->env);
                    }
                }
            }
        }
        unsigned int GC_LIMIT;
    public:
        GarbageCollector() {
            GC_LIMIT = 512 * sizeof(ActivationRecord);
        }
        bool ready() {
            return (alloc.getLiveList().size() * sizeof(ActivationRecord)) > GC_LIMIT;
        }
        void run(ActivationRecord* callstk, ActivationRecord* globals, StackItem opstk[], int sp, ConstPool* constPool) {
            markOpStack(opstk, sp);
            markAR(callstk);
            markAR(globals);
            markConstPool(constPool);
            sweep();
            GC_LIMIT *= 2;
        }
};


#endif