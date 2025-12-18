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
                    //cout<<"L";
                    for (auto & it : *curr->list) {
                        markItem(&it);
                    }
                } else if (curr->type == CLOSURE && curr->closure != nullptr) {
                    //cout<<"&";
                    markAR(curr->closure->env);
                } else if (curr->type == CLASS && curr->object != nullptr && curr->object->instantiated) {
                    //cout<<"@";
                    for (auto & it : curr->object->fields) {
                        markItem(&it.second);
                    }
                } else if (curr->type == STRING && curr->strval != nullptr) {
                    //cout<<"$"<<endl;
                }
            }
        }
        void markItem(StackItem* si) {
            //cout<<",";
            if (si->type == OBJECT) {
                markObject(si->objval);
                //cout<<".";
            }
        }
        void markAR(ActivationRecord* callframe) {
            ActivationRecord* ar = callframe;
            //cout<<"#";
            if (ar != nullptr && !ar->marked) {
                //cout<<"*: ";
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
            unordered_set<GCObject*> toFree;
            for (auto & it : alloc.getLiveList()) {
                if (it->marked) {
                    it->marked = false;
                    nextGen.insert(it);
                } else {
                    toFree.insert(it);
                }
            }
            //cout<<"Collecting: "<<toFree.size()<<endl;
            for (auto & it : toFree) {
                if (it->isAR) {
                    // cout<<"Free AR."<<endl;
                    freeAR(it);
                } else {
                    //cout<<"Free item"<<endl;
                    alloc.free((GCItem*)it);
                }
            }
            //cout<<alloc.getLiveList().size()<<", "<<toFree.size()<<", "<<nextGen.size()<<endl;
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
        void freeAR(GCObject* to) {
            if (to != nullptr) {
                delete to;
            }
        }
        int GC_LIMIT;
    public:
        GarbageCollector() {
            GC_LIMIT = 512;
        }
        bool ready() {
            return alloc.getLiveList().size() > GC_LIMIT;
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