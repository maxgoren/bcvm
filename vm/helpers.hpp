#ifndef helpers_hpp
#define helpers_hpp
#include <iostream>
#include <vector>
#include <unordered_map>
#include "heapitem.hpp"
#include "stackitem.hpp"
using namespace std;

void freeListObject(GCItem* item) {
    for (auto it : *item->list) {
        if (it.type == OBJECT) {
            free(it.objval);
        }
    }
    delete item->list;
}

void freeClassObject(GCItem* item) {
    for (auto it : item->object->fields) {
        if (it.second.type == OBJECT)
            free(it.second.objval);
    }
    delete item->object;
}

#endif