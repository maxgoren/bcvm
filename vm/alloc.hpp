#ifndef alloc_hpp
#define alloc_hpp
#include <iostream>
#include <unordered_set>
#include <deque>
#include "heapitem.hpp"
using namespace std;


class GCAllocator {
    private:
        friend class GarbageCollector;
        unordered_set<GCObject*> live_items;
        deque<GCItem*> free_list;
        GCItem* next() {
            if (free_list.empty()) {
                return new GCItem();
            }
            GCItem* x = free_list.front(); 
            free_list.pop_front();
            x->marked = false;
            x->isAR = false;
            return x;
        }
    public:
        GCAllocator() {

        }
        void free(GCItem* item) {
            if (item == nullptr)
                return;
            switch (item->type) {
                case STRING: {
                    if (item->strval)
                        delete item->strval;
                } break;
                case LIST: {
                   if (item->list)
                        delete item->list;
                } break;
                case CLOSURE: {
                    freeClosure(item->closure);
                } break;
                    case CLASS: {
                    freeClass(item->object);
                } break;
            };
            item->type = NILPTR;
            free_list.push_back(item);
        }
        GCItem* alloc(string* s) {
            GCItem* x = next();
            x->type = STRING;
            x->strval = s;
            live_items.insert(x);
            return x;
        }
        GCItem* alloc(Closure* c) {
            GCItem* x = next();
            x->type = CLOSURE;
            x->closure = c;
            live_items.insert(x);
            return x;
        }
        GCItem* alloc(deque<StackItem>* l) {
            GCItem* x = next();
            x->type = LIST;
            x->list = l;
            live_items.insert(x);
            return x;
        }
        GCItem* alloc(ClassObject* l) {
            GCItem* x = next();
            x->type = CLASS;
            x->object = l;
            live_items.insert(x);
            return x;
        }
        unordered_set<GCObject*>& getLiveList() {
            return live_items;
        }
};

GCAllocator alloc;

#endif