#ifndef scopingst_hpp
#define scopingst_hpp
#include "../vm/stackitem.hpp"
#include "../vm/constpool.hpp"
#include <map>
#include <vector>
#include <iostream>
using namespace std;

const unsigned int MAX_LOCALS = 255;

struct Scope;

enum SymTableType {
    LOCALVAR = 1,
    FUNCVAR = 2,
    CLASSVAR = 3
};

struct SymbolTableEntry {
    string name;
    int type;
    int addr;
    int depth;
    int constPoolIndex;
    int lineNum;
    SymbolTableEntry(string n, int adr, int cpi, int t, int d) : type(t), addr(adr), name(n), depth(d), constPoolIndex(cpi), lineNum(0) { }
    SymbolTableEntry(string n, int adr, int d) : type(1), name(n), addr(adr), depth(d), constPoolIndex(-1), lineNum(0) { }
    SymbolTableEntry() : type(0), addr(-1), constPoolIndex(-1), lineNum(0) { }
    SymbolTableEntry(const SymbolTableEntry& e) {
        name = e.name;
        type = e.type;
        addr = e.addr;
        depth = e.depth;
        constPoolIndex = e.constPoolIndex;
        lineNum = e.lineNum;
    }
    SymbolTableEntry& operator=(const SymbolTableEntry& e) {
        if (this != &e) {
            name = e.name;
            type = e.type;
            addr = e.addr;
            depth = e.depth;
            constPoolIndex = e.constPoolIndex;
            lineNum == e.lineNum;
        }
        return *this;
    }
    bool operator==(const SymbolTableEntry& st) const {
        return name == st.name && type == st.type && addr == st.addr && lineNum == st.lineNum;
    }
    bool operator!=(const SymbolTableEntry& st) const {
        return !(*this==st);
    }
};


struct BlockScope {
    SymbolTableEntry data[MAX_LOCALS];
    int n = 0;
    BlockScope() {
        n = 0;
    }
    int size() {
        return n;
    }
    void insert(string name, SymbolTableEntry st) {
        cout<<st.name<<" added with address "<<st.addr<<" in positon "<<n<<endl;
        data[n++] = st;
    }
    SymbolTableEntry& find(string name) {
        int i = n-1;
        while (i >= 0) {
            if (data[i].name == name) {
                return data[i];
            }
            i--;
        }
        return end();
    }
    SymbolTableEntry& end() {
        return data[n];
    }
    SymbolTableEntry& operator[](string name) {
        for (int i = n-1; i >= 0; i--) {
            if (data[i].name == name) {
                return data[i];
            }
        }
        data[n] = SymbolTableEntry(name, n, -1);
        n++;
        return data[n-1];
    }
};

struct Scope {
    Scope* enclosing;
    BlockScope symTable;
};

class ScopingST {
    private:
        Scope* st;
        ConstPool constPool;
        SymbolTableEntry nfSentinel;
        unordered_map<string, ClassObject*> objectDefs;
        int nextAddr() {
            int na = st->symTable.size()+1;
            return na;
        }
        int depth(Scope* s) {
            if (s == nullptr || s->enclosing == nullptr) {
                cout<<"Rnnnt."<<endl;
                return -1;
            }
            auto x = s;
            int d = 0;
            while (x != nullptr) {
                d++;
                x = x->enclosing;
            }
            return d-1;
        }
    public:
        ScopingST() {
            st = new Scope();
            st->enclosing = nullptr;
            nfSentinel = SymbolTableEntry("not found", -1, -1);
        }
        ConstPool& getConstPool() {
            return constPool;
        }
        void openObjectScope(string name) {
            if (st->symTable.find(name) != st->symTable.end()) {
                 if (st->symTable.find(name).type == CLASSVAR) {
                    cout<<"Reopened Object scope for "<<name<<endl;
                    Scope* ns = constPool.get(st->symTable.find(name).constPoolIndex).objval->object->scope;
                    st = ns;
                 } else {
                    Scope* ns = new Scope();
                    ns->enclosing = st;
                    st = ns;
                 }
            } else {
                Scope* ns = new Scope;
                ns->enclosing = st;
                ClassObject* obj = new ClassObject(name, ns);
                objectDefs.insert(make_pair(name, obj));
                int constIdx = constPool.insert(gc.alloc(obj));
                int envAddr = nextAddr();
                objectDefs[name]->cpIdx = constIdx;
                st->symTable.insert(name, SymbolTableEntry(name, envAddr, constIdx, CLASSVAR, depth(st)+1));
                st = ns;
            }
        }
        void copyObjectScope(string instanceName, string objName) {
            Scope* sc = objectDefs[objName]->scope;
            st->symTable.insert(instanceName, SymbolTableEntry(instanceName, nextAddr(), objectDefs[objName]->cpIdx, CLASSVAR, depth(st)+1));
        }
        void openFunctionScope(string name, int L1) {
            if (st->symTable.find(name) != st->symTable.end()) {
                Scope* ns = constPool.get(st->symTable[name].constPoolIndex).objval->closure->func->scope;
                constPool.get(st->symTable[name].constPoolIndex).objval->closure->func->start_ip = L1;
                st = ns;
            } else {
                Scope*  ns = new Scope;
                ns->enclosing = st;
                Function* f = makeFunction(name, L1, ns);
                int constIdx = constPool.insert(new Closure(f));
                int envAddr = nextAddr();
                st->symTable.insert(name, SymbolTableEntry(name, envAddr, constIdx, FUNCVAR, depth(st)+1));
                st = ns;
            }
        }
        void closeScope() {
            if (st != nullptr && st->enclosing != nullptr) {
                st = st->enclosing;
            }
        }
        void insert(string name) {
            st->symTable[name] = SymbolTableEntry(name, nextAddr(), depth(st));
        }
        bool existsInScope(string name) {
            return st->symTable.find(name) != st->symTable.end();
        }
        SymbolTableEntry& lookup(string name, int lineNum) {
            Scope* x = st;
            while (x != nullptr) {
                if (x->symTable.find(name) != x->symTable.end())
                    return x->symTable[name];
                x = x->enclosing;
            }
            cout<<"lookup "<<name<<" failed"<<endl;
            return nfSentinel;
        }
        ClassObject* lookupClass(string name) {
            if (objectDefs.find(name) != objectDefs.end())
                return objectDefs.at(name);
            return nullptr;
        }
        int depth() {
            return depth(st);
        }
        void printScope(Scope* s, int d) {
            auto x = s;
            for (int i = 0; i < x->symTable.size(); i++) {
                auto m = x->symTable.data[i];
                for (int i = 0; i < d; i++) cout<<"  ";
                cout<<m.name<<": "<<m.addr<<", "<<m.depth<<endl;
                if (m.type == 2) {
                    printScope(constPool.get(m.constPoolIndex).objval->closure->func->scope,d + 1);
                } else if (m.type == 3) {
                    printScope(constPool.get(m.constPoolIndex).objval->object->scope, d+1);
                }
            }
        }
        void print() {
            cout<<"Symbol table: \n";
            printScope(st, 1);
        }
};

#endif