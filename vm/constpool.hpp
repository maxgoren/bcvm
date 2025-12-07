#ifndef constpool_hpp
#define constpool_hpp
#include <unordered_map>
#include "stackitem.hpp"
using namespace std;

class ConstPool {
    private:
        unordered_map<string, int> stringPool;
        StackItem* data;
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
            if (item.type == OBJECT && item.objval->type == STRING) {
                cout<<"Check that striggidy string piz first"<<endl;
                if (stringPool.find(*item.objval->strval) != stringPool.end()) {
                    cout<<"Bingo, bitch."<<endl;
                    return stringPool.at(*item.objval->strval);
                }
            }
            data[n++] = item;
            if (item.type == OBJECT && item.objval->type == STRING) {
                stringPool.insert(make_pair(*item.objval->strval, n-1));
            }
            return n-1;
        }
        StackItem& get(int indx) {
            return data[indx];
        }
        int size() {
            return n;
        }
};

#endif