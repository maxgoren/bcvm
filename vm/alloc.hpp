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
        unordered_set<GCItem*> live_items;
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
                cout<<"From free list"<<endl;
            } else {
                x = new GCItem(s);
            }
            live_items.insert(x);
            return x;
        }
        GCItem* alloc(Closure* c) {
            GCItem* x;
            if (!free_list.empty()) {
                x = free_list.back(); 
                free_list.pop_back();
                x->type = CLOSURE;
                x->closure = c;
                cout<<"From free list"<<endl;
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
                cout<<"From free list"<<endl;
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
                cout<<"From free list"<<endl;
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
                cout<<"From free list"<<endl;
            } else {
                x = new GCItem(l);
            }
            live_items.insert(x);
            return x;
        }
        unordered_set<GCItem*>& getLiveList() {
            return live_items;
        }
        list<GCItem*> getFreeList() {
            return free_list;
        }
};


#endif