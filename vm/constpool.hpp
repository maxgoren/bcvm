#ifndef constpool_hpp
#define constpool_hpp
#include <unordered_map>
#include <queue>
#include "stackitem.hpp"
#include "closure.hpp"
using namespace std;

// the items stored in this table are _not_ considered 
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
    public:
        ConstPool() {
            n = 0;
            maxN = 255;
            data = new StackItem[maxN];
        }
        ~ConstPool() {
            for (int i = 0; i < maxN; i++) {
                if (data[i].type == OBJECT) {
                    alloc.free(data[i].objval);
                }
            }
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
            if (item.type == OBJECT && item.objval->type == STRING) {
                strval = item.toString();
                if (stringPool.find(strval) != stringPool.end()) {
                    return stringPool.at(strval);
                }
            }
            int addr = nextAddress();
            data[addr] = item;
            if (item.type == OBJECT && item.objval->type == STRING) {
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