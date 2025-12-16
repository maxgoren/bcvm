#ifndef alloc_hpp
#define alloc_hpp
#include <iostream>
#include <unordered_set>
#include <list>
#include <deque>
#include "heapitem.hpp"
using namespace std;


class GCAllocator {
    private:
        friend class GarbageCollector;
        unordered_set<GCObject*> live_items;
        list<GCItem*> free_list;
    public:
        GCAllocator() {

        }
        GCItem* alloc(string* s) {
            GCItem* x;
            if (!free_list.empty()) {
                x = free_list.back(); 
                free_list.pop_back();
                x->type = STRING;
                x->strval = s;
            } else {
                x = new GCItem(s);
            }
            live_items.insert(x);
            return x;
        }
        void free(GCItem* item) {
            if (item == nullptr)
                return;
            switch (item->type) {
                case STRING: {
                    //cout<<"Free string: "<<item->toString()<<endl;
                    delete item->strval;
                } break;
                case CLASS: {
                    //cout<<"Free class."<<endl;
                    freeClass(item->object);
                } break;
                case LIST: {
                   //cout<<"Free list: "<<item->toString()<<endl;
                   delete item->list;
                } break;
                case CLOSURE: {
                    //cout<<"Free closure: "<<item->toString()<<endl;
                    item->closure = nullptr;
                } break;
                case FUNCTION: {
                    //cout<<"Free function: "<<item->toString()<<endl;
                    delete item->func;
                }
            };
            item->type = NILPTR;
            free_list.push_back(item);
        }
        GCItem* alloc(Closure* c) {
            GCItem* x;
            if (!free_list.empty()) {
                x = free_list.back(); 
                free_list.pop_back();
                x->type = CLOSURE;
                x->closure = c;
            } else {
                x = new GCItem(c);
            }
            live_items.insert(x);
            return x;
        }
        GCItem* alloc(Function* f) {
            GCItem* x;
            if (!free_list.empty()) {
                x = free_list.back(); 
                free_list.pop_back();
                x->type = FUNCTION;
                x->func = f;
            } else {
                x = new GCItem(f);
            }
            live_items.insert(x);
            return x;
        }
        GCItem* alloc(deque<StackItem>* l) {
            GCItem* x;
            if (!free_list.empty()) {
                x = free_list.back(); 
                free_list.pop_back();
                x->type = LIST;
                x->list = l;
            } else {
                x = new GCItem(l);
            }
            live_items.insert(x);
            return x;
        }
        GCItem* alloc(ClassObject* l) {
            GCItem* x;
            if (!free_list.empty()) {
                x = free_list.back(); 
                free_list.pop_back();
                x->type = CLASS;
                x->object = l;
            } else {
                x = new GCItem(l);
            }
            live_items.insert(x);
            return x;
        }
        unordered_set<GCObject*>& getLiveList() {
            return live_items;
        }
        list<GCItem*> getFreeList() {
            return free_list;
        }
};

GCAllocator alloc;

#endif