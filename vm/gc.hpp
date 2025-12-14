#ifndef gc_hpp
#define gc_hpp
#include <vector>
#include <memory>
#include "constpool.hpp"
#include "stackitem.hpp"
#include "instruction.hpp"
using namespace std;

void markObject(GCItem* object) {
    if (object == nullptr)
        return;
    vector<GCItem*> sf;
    sf.push_back(object);
    while (!sf.empty()) {
        GCItem* curr = sf.back(); sf.pop_back();
        if (curr != nullptr && curr->marked == false) {
            curr->marked = true;
            switch (curr->type) {
                case STRING: { cout<<"Marked string: ["<<*curr->strval<<"]"<<endl; } break;
                case FUNCTION: { cout<<"Marked function: ["<<curr->func->name<<"]"<<endl; } break;
                case CLOSURE: {
                    cout<<"Marked closure"<<endl;
                    for (int i = 0; i < 255; i++) {
                        if (curr->closure->env->locals[i].type == OBJECT) {
                            sf.push_back(curr->closure->env->locals[i].objval);
                        }
                    }
                } break;
                case LIST: {      
                    cout<<"Marked list: "<<curr->toString()<<endl;
                    for (auto & it : *curr->list) {
                        if (it.type == OBJECT && it.objval != nullptr) {
                            sf.push_back(it.objval);
                        }
                    }
                } break;
                default:
                    break;
            }
        }   
    }
}

class GarbageCollector {
    private:
        int GC_LIMIT;
    public:
        GarbageCollector() {
            GC_LIMIT = 50;
        }
        void mark(ActivationRecord* callstk, ConstPool& constPool) {
            cout<<"start mark phase"<<endl;
            for (auto & it = callstk; it != nullptr; it = it->control) {
                for (int i = 0; i < 255; i++) {
                    if (it->locals[i].type == OBJECT) {
                        markObject(it->locals[i].objval);
                    }
                }
            }
            constPool.cleanTable();
            cout<<"Marking complete."<<endl;
        }
        void sweep() {
            unordered_set<GCItem*> nextGen;
            unordered_set<GCItem*> toFree;
            cout<<"Collecting: "<<toFree.size()<<endl;
            for (auto it : toFree) {
            }
            GC_LIMIT *= 2;
        }
        void run(ActivationRecord* callstk, ConstPool& constPool) {
            mark(callstk, constPool);
            sweep();
        }
};


#endif