#ifndef constpool_hpp
#define constpool_hpp
#include <unordered_map>
#include <queue>
#include "stackitem.hpp"
using namespace std;

class WeakReference {
    private:
};

class ConstPool {
    private:
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
        void cleanTable() {
            for (int i = 0; i < n; i++) {
                if (data[i].type == OBJECT && data[i].objval != nullptr) {
                    if (!data[i].objval->marked) {
                        freeList.push(i);
                        data[i].type = NIL;
                        gc.free(data[i].objval);
                    }
                }
            }
        }
};

#endif