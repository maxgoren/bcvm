#ifndef scopingst_hpp
#define scopingst_hpp
#include "../vm/stackitem.hpp"
#include <map>
#include <vector>
#include <iostream>
using namespace std;

struct Scope;

struct SymbolTableEntry {
    string name;
    int type;
    int addr;
    int depth;
    int constPoolIndex;
    SymbolTableEntry(string n, int adr, int cpi, int d) : type(2), addr(adr), name(n), depth(d), constPoolIndex(cpi) { }
    SymbolTableEntry(string n, int adr, int d) : type(1), name(n), addr(adr), depth(d), constPoolIndex(-1) { }
    SymbolTableEntry() : type(0), addr(-1), constPoolIndex(-1) { }
    SymbolTableEntry(const SymbolTableEntry& e) {
        name = e.name;
        type = e.type;
        addr = e.addr;
        depth = e.depth;
        constPoolIndex = e.constPoolIndex;
    }
    SymbolTableEntry& operator=(const SymbolTableEntry& e) {
        if (this != &e) {
            name = e.name;
            type = e.type;
            addr = e.addr;
            depth = e.depth;
            constPoolIndex = e.constPoolIndex;
        }
        return *this;
    }
    bool operator==(const SymbolTableEntry& st) const {
        return name == st.name && type == st.type && addr == st.addr;
    }
    bool operator!=(const SymbolTableEntry& st) const {
        return !(*this==st);
    }
};


struct BlockScope {
    SymbolTableEntry data[255];
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

//{ fn ok() { println "hi"; } ok(); }
class ScopingST {
    private:
        Scope* st;
        ConstPool constPool;
        SymbolTableEntry nfSentinel;
        int nextAddr() {
            int na = st->symTable.size()+1;
            //cout<<"Generated: "<<na<<" as next address."<<endl;
            return na;
        }
        int depth(Scope* s) {
            if (s->enclosing == nullptr)
                return -1;
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
        void openFunctionScope(string name, int L1) {
            //cout<<"Open scope "<<name<<endl;
            if (st->symTable.find(name) != st->symTable.end()) {
               // cout<<"Reopening existing for "<<st->symTable[name].addr<<endl;
                Scope* ns = constPool.get(st->symTable[name].constPoolIndex).closure->func->scope;
                constPool.get(st->symTable[name].constPoolIndex).closure->func->start_ip = L1;
                st = ns;
            } else {
                Scope*  ns = new Scope;
                ns->enclosing = st;
                Function* f = makeFunction(name, L1, ns);
                int idx = constPool.insert(new Closure(f));
                int addr = nextAddr();
               // cout<<"Adding to enclosing at "<<addr<<endl;;
                st->symTable.insert(name, SymbolTableEntry(name, addr, idx, depth(ns)));
                st = ns;
            }
        }
        void closeScope() {
            if (st->enclosing != nullptr) {
                //cout<<"Closing scope"<<endl;
                st = st->enclosing;
            }
        }
        void insert(string name) {
            st->symTable[name] = SymbolTableEntry(name, nextAddr(), depth(st));
        }
        bool existsInScope(string name) {
            return st->symTable.find(name) != st->symTable.end();
        }
        SymbolTableEntry& lookup(string name) {
            Scope* x = st;
            while (x != nullptr) {
                if (x->symTable.find(name) != x->symTable.end())
                    return x->symTable[name];
                x = x->enclosing;
            }
            cout<<"lookup "<<name<<" failed"<<endl;
            return nfSentinel;
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
                    printScope(constPool.get(m.constPoolIndex).closure->func->scope,d + 1);
                }
            }
        }
        void print() {
            cout<<"Symbol table: \n";
            printScope(st, 1);
        }
};

#endif