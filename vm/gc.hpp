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
        void markObject(GCItem* object) {
            if (object == nullptr)
                return;
            vector<GCItem*> sf;
            sf.push_back(object);
            while (!sf.empty()) {
                GCItem* curr = sf.back(); sf.pop_back();
                if (curr != nullptr && curr->marked == false) {
                    curr->marked = true;
                    if (curr->type == LIST && curr->list != nullptr) {
                        for (auto & it : *curr->list) {
                            if (it.type == OBJECT && it.objval != nullptr) {
                                sf.push_back(it.objval);
                            }
                        }
                    } else if (curr->type == CLOSURE && curr->closure != nullptr) {
                        markAR(curr->closure->env, 1);
                    } else if (curr->type == CLASS && curr->object != nullptr) {
                        for (auto & it : curr->object->fields) {
                            markItem(&it.second);
                        }
                    }
                }
            }
        }
        void markItem(StackItem* si) {
            if (si->type == OBJECT) {
                markObject(si->objval);
            }
        }
        void markAR(ActivationRecord* callframe, int d) {
            ActivationRecord* ar = callframe;
            if (ar && !ar->marked) {
                ar->marked = true;
                for (int i = 0; i < d; i++) cout<<" ";
                cout<<ar->cp_index<<endl;
                for (int i = 0; i < 255; i++) {
                    markItem(&ar->locals[i]);
                }
                markAR(ar->access, d + 1);
                if (ar->control != ar->access)
                    markAR(ar->control, d + 2);
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
            GC_LIMIT = 255;
        }
        bool ready() {
            return alloc.getLiveList().size() > GC_LIMIT;
        }
        void sweep() {
            unordered_set<GCObject*> nextGen;
            unordered_set<GCObject*> toFree;
            for (auto it : alloc.getLiveList()) {
                if (it->marked) {
                    nextGen.insert(it);
                } else {
                    toFree.insert(it);
                }
            }
            cout<<"Collecting: "<<toFree.size()<<endl;
            for (auto & it : toFree) {
                if (!it->isAR)
                    alloc.free((GCItem*)it);
                else freeAR(it);
            }
            cout<<alloc.getLiveList().size()<<", "<<toFree.size()<<", "<<nextGen.size()<<endl;
            alloc.getLiveList().swap(nextGen);
            GC_LIMIT *= 2;
        }
        void run(ActivationRecord* callstk, ActivationRecord* globals, ConstPool& constPool) {
            cout<<"start mark phase"<<endl;
            cout<<"Marking Callstack: "<<endl;
            markAR(callstk, 1);
            markAR(globals, 1);
            cout<<"Checking const table: "<<endl;
            constPool.cleanTable();
            cout<<"Marking complete."<<endl;
            sweep();
        }
};


#endif