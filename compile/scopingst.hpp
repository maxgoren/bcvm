#ifndef scopingst_hpp
#define scopingst_hpp
#include <iostream>
#include <vector>
#include <map>
#include "../vm/stackitem.hpp"
#include "../vm/constpool.hpp"
#include "../vm/callframe.hpp"
#include "../vm/closure.hpp"
using namespace std;

const unsigned int MAX_LOCALS = 255;

struct Scope;

enum SymTableType {
    NONE = 0,
    LOCALVAR = 1,
    FUNCVAR = 2,
    CLASSVAR = 3
};

struct SymbolTableEntry {
    string name;
    SymTableType type;
    int addr;
    int depth;
    int constPoolIndex;
    int lineNum;
    SymbolTableEntry(string n, int adr, int cpi, SymTableType t, int d) : type(t), addr(adr), name(n), depth(d), constPoolIndex(cpi), lineNum(0) { }
    SymbolTableEntry(string n, int adr, int d) : type(LOCALVAR), name(n), addr(adr), depth(d), constPoolIndex(-1), lineNum(0) { }
    SymbolTableEntry() : type(NONE), addr(-1), constPoolIndex(-1), lineNum(0) { }
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

class BlockScopeIterator {
    private:
        SymbolTableEntry* cpos;
        int ipos;
        int fpos;
    public:
        BlockScopeIterator(SymbolTableEntry* s, int n) {
            cpos = s;
            ipos = 0;
            fpos = n;
        }
        bool done() {
            return ipos == fpos;
        }
        SymbolTableEntry& get() {
            return cpos[ipos];
        }
        void next() {
            ipos++;
        }
};


class BlockScope {
    private:
        friend class ScopingST;
        SymbolTableEntry data[MAX_LOCALS];
        int n;
        BlockScope* enclosingScope;
    public:
        BlockScope(BlockScope* parent = nullptr) {
            n = 0;
            enclosingScope = parent;
        }
        int size() {
            return n;
        }
        void insert(string name, SymbolTableEntry st) {
            data[n++] = st;
        }
        SymbolTableEntry& find(string name) {
            int idx = 0;
            while (idx < n) {
                if (data[idx].name == name) {
                    return data[idx];
                }
                idx++;
            }
            return end();
        }
        SymbolTableEntry& end() {
            return data[n];
        }
        SymbolTableEntry& operator[](string name) {
            if (find(name) == end())
                insert(name, SymbolTableEntry(name, n, -1));
            return find(name);
        }
        BlockScopeIterator iter() {
            return BlockScopeIterator(data, n);
        }
        BlockScope* getEnclosing() {
            return enclosingScope;
        }
};

class ScopingST {
    private:
        BlockScope* currentScope;
        ConstPool constPool;
        SymbolTableEntry nfSentinel;
        unordered_map<string, ClassObject*> objectDefs;
        int nextAddr() {
            int na = currentScope->size()+1;
            return na;
        }
        int depth(BlockScope* s) {
            if (s == nullptr || s->getEnclosing() == nullptr) {
                return -1;
            }
            auto x = s;
            int d = 0;
            while (x != nullptr) {
                d++;
                x = x->getEnclosing();
            }
            return d-1;
        }
        void printST(BlockScope* s, int d) {
            auto x = s;
            for (int i = 0; i < MAX_LOCALS; i++) {
                auto m = x->data[i];
                if (m.type != NONE) {
                    for (int i = 0; i < d; i++) cout<<"  ";
                    cout<<m.name<<": "<<m.addr<<", "<<m.depth<<endl;
                    if (m.type == 2) {
                        printST(constPool.get(m.constPoolIndex).objval->closure->func->scope,d + 1);
                    } else if (m.type == 3) {
                        printST(constPool.get(m.constPoolIndex).objval->object->scope, d+1);
                    }
                }
            }
        }
    public:
        ScopingST() {
            currentScope = new BlockScope();
            nfSentinel = SymbolTableEntry("not found", -1, -1);
        }
        ConstPool& getConstPool() {
            return constPool;
        }
        void openObjectScope(string name) {
            if (currentScope->find(name) != currentScope->end()) {
                 if (currentScope->find(name).type == CLASSVAR) {
                    BlockScope* ns = constPool.get(currentScope->find(name).constPoolIndex).objval->object->scope;
                    currentScope = ns;
                 } else {
                    BlockScope* ns = new BlockScope(currentScope);
                    currentScope = ns;
                 }
            } else {
                BlockScope* ns = new BlockScope(currentScope);
                ClassObject* obj = new ClassObject(name, ns);
                objectDefs.insert(make_pair(name, obj));
                int constIdx = constPool.insert(alloc.alloc(obj));
                int envAddr = nextAddr();
                objectDefs[name]->cpIdx = constIdx;
                currentScope->insert(name, SymbolTableEntry(name, envAddr, constIdx, CLASSVAR, depth(currentScope)+1));
                currentScope = ns;
            }
        }
        void copyObjectScope(string instanceName, string objName) {
            currentScope->insert(instanceName, SymbolTableEntry(instanceName, nextAddr(), objectDefs[objName]->cpIdx, CLASSVAR, depth(currentScope)+1));
        }
        void openFunctionScope(string name, int L1) {
            if (currentScope->find(name) != currentScope->end()) {
                BlockScope* ns = constPool.get(currentScope->find(name).constPoolIndex).objval->closure->func->scope;
                constPool.get(currentScope->find(name).constPoolIndex).objval->closure->func->start_ip = L1;
                currentScope = ns;
            } else {
                BlockScope*  ns = new BlockScope(currentScope);
                int funcId = constPool.insert(alloc.alloc(new Function(name, L1, ns)));
                int constIdx = constPool.insert(alloc.alloc(new Closure(constPool.get(funcId).objval->func, nullptr)));
                int envAddr = nextAddr();
                currentScope->insert(name, SymbolTableEntry(name, envAddr, constIdx, FUNCVAR, depth(currentScope)+1));
                currentScope = ns;
            }
        }
        void closeScope() {
            if (currentScope != nullptr && currentScope->getEnclosing() != nullptr) {
                currentScope = currentScope->getEnclosing();
            }
        }
        void insert(string name) {
            currentScope->insert(name, SymbolTableEntry(name, nextAddr(), depth(currentScope)));
        }
        bool existsInScope(string name) {
            return currentScope->find(name) != currentScope->end();
        }
        SymbolTableEntry& lookup(string name) {
            BlockScope* x = currentScope;
            while (x != nullptr) {
                if (x->find(name) != x->end())
                    return x->find(name);
                x = x->getEnclosing();
            }
            return nfSentinel;
        }
        ClassObject* lookupClass(string name) {
            if (objectDefs.find(name) != objectDefs.end())
                return objectDefs.at(name);
            return nullptr;
        }
        int depth() {
            return depth(currentScope);
        }
        void print() {
            cout<<"Symbol table: \n";
            printST(currentScope, 1);
        }
};

#endif