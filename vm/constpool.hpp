#ifndef constpool_hpp
#define constpool_hpp
#include <unordered_map>
#include <queue>
#include "stackitem.hpp"
#include "closure.hpp"
using namespace std;

// The cleanTable() method should only be called by the garbage collector. It is meant to be
// called after the mark phase && right before the sweep phase thus turning this into a table 
// of so-called "weak references". As such the items stored in this table are _not_ considered 
// roots for the GC. If an object was not reached during the mark phase it should be reclaimed 
// as garbage

class ConstPool {
    private:
    friend class GarbageCollector;
        unordered_map<string, int> stringPool;
        StackItem* data;
        queue<int> freeList;
        int n;
        int maxN;
        void grow() {
            auto tmp = data;
            data = new StackItem[2*maxN];
            for (int i = 0; i < n; i++) {
                data[i] = tmp[i];
            }
            delete [] tmp;
            maxN *= 2;
        }
        int nextAddress() {
            if (!freeList.empty()) {
                int next = freeList.front();
                freeList.pop();
                return next;
            }
            if (n+1 == maxN)
                grow();
            int next = n;
            n += 1;
            return next;
        }
        void cleanTable() {
            for (int i = 0; i < maxN; i++) {
                if (data[i].type == OBJECT && data[i].objval != nullptr) {
                    if (!data[i].objval->marked) {
                        if (data[i].objval->type == CLOSURE) {
                            data[i].objval->marked = true;
                            cout<<"MTBTTF"<<endl;
                        } else {
                            cout<<"Freeing "<<i<<" from constPool"<<endl;
                            freeList.push(i);
                            data[i].type = NIL;
                            alloc.free(data[i].objval);
                        }
                    }
                }
            }
        }
    public:
        ConstPool() {
            n = 0;
            maxN = 255;
            data = new StackItem[maxN];
        }
        ~ConstPool() {
            delete [] data;
        }
        ConstPool(const ConstPool& cp) {
            n = cp.n;
            maxN = cp.maxN;
            data = new StackItem[maxN];
            for (int i = 0; i < n; i++)
                data[i] = cp.data[i];
            stringPool = cp.stringPool;
        }
        ConstPool& operator=(const ConstPool& cp) {
            if (this != &cp) {
                n = cp.n;
                maxN = cp.maxN;
                data = new StackItem[maxN];
                for (int i = 0; i < n; i++)
                    data[i] = cp.data[i];
                stringPool = cp.stringPool;
            }
            return *this;
        }
        int insert(StackItem item) {
            string strval;
            if (item.type == NUMBER || (item.type == OBJECT && item.objval->type == STRING)) {
                strval = item.toString();
                if (stringPool.find(strval) != stringPool.end()) {
                    return stringPool.at(strval);
                }
            }
            int addr = nextAddress();
            data[addr] = item;
            if (item.type == NUMBER || (item.type == OBJECT && item.objval->type == STRING)) {
                if (!strval.empty()) {
                    stringPool.insert(make_pair(strval, addr));
                }
            }
            return addr;
        }
        StackItem& get(int indx) {
            return data[indx];
        }
        int size() {
            return n;
        }
};

#endif