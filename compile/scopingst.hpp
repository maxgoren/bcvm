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


class BlockScope {
    private:
        friend class ScopingST;
        int hash(string n) {
            int h = 3178;
            for (char c : n) {
                h = (h << 27) | (h >> 5);
                h = (33*h+(int)c);
            }
            
            return (h % MAX_LOCALS) + 1;
        }
        SymbolTableEntry data[MAX_LOCALS];
        int n;
    public:
        BlockScope() {
            n = 0;
        }
        int size() {
            return n;
        }
        void insert(string name, SymbolTableEntry st) {
            int idx = hash(name);
            while (data[idx].type != NONE) {
                if (idx != 0 && data[idx].name == name) {
                    break;
                }
                idx = (idx + 1) % MAX_LOCALS;
            }
            data[idx] = st;
            n++;
        }
        SymbolTableEntry& find(string name) {
            int idx = hash(name);
            while (data[idx].type != NONE) {
                if (data[idx].name == name) {
                    return data[idx];
                }
                idx = (idx + 1) % MAX_LOCALS;
            }
            return end();
        }
        SymbolTableEntry& end() {
            return data[0];
        }
        SymbolTableEntry& operator[](string name) {
            if (find(name) == end())
                insert(name, SymbolTableEntry(name, n, -1));
            return find(name);
        }
};

struct Scope {
    Scope* enclosing;
    BlockScope symTable;
};

class ScopingST {
    private:
        Scope* currentScope;
        ConstPool constPool;
        SymbolTableEntry nfSentinel;
        unordered_map<string, ClassObject*> objectDefs;
        int nextAddr() {
            int na = currentScope->symTable.size()+1;
            return na;
        }
        int depth(Scope* s) {
            if (s == nullptr || s->enclosing == nullptr) {
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
        void printST(Scope* s, int d) {
            auto x = s;
            for (int i = 0; i < MAX_LOCALS; i++) {
                auto m = x->symTable.data[i];
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
            currentScope = new Scope();
            currentScope->enclosing = nullptr;
            nfSentinel = SymbolTableEntry("not found", -1, -1);
        }
        ConstPool& getConstPool() {
            return constPool;
        }
        void openObjectScope(string name) {
            if (currentScope->symTable.find(name) != currentScope->symTable.end()) {
                 if (currentScope->symTable.find(name).type == CLASSVAR) {
                    Scope* ns = constPool.get(currentScope->symTable.find(name).constPoolIndex).objval->object->scope;
                    currentScope = ns;
                 } else {
                    Scope* ns = new Scope();
                    ns->enclosing = currentScope;
                    currentScope = ns;
                 }
            } else {
                Scope* ns = new Scope;
                ns->enclosing = currentScope;
                ClassObject* obj = new ClassObject(name, ns);
                objectDefs.insert(make_pair(name, obj));
                int constIdx = constPool.insert(alloc.alloc(obj));
                int envAddr = nextAddr();
                objectDefs[name]->cpIdx = constIdx;
                currentScope->symTable.insert(name, SymbolTableEntry(name, envAddr, constIdx, CLASSVAR, depth(currentScope)+1));
                currentScope = ns;
            }
        }
        void copyObjectScope(string instanceName, string objName) {
            currentScope->symTable.insert(instanceName, SymbolTableEntry(instanceName, nextAddr(), objectDefs[objName]->cpIdx, CLASSVAR, depth(currentScope)+1));
        }
        void openFunctionScope(string name, int L1) {
            if (currentScope->symTable.find(name) != currentScope->symTable.end()) {
                Scope* ns = constPool.get(currentScope->symTable[name].constPoolIndex).objval->closure->func->scope;
                constPool.get(currentScope->symTable[name].constPoolIndex).objval->closure->func->start_ip = L1;
                currentScope = ns;
            } else {
                Scope*  ns = new Scope;
                ns->enclosing = currentScope;
                int constIdx = constPool.insert(alloc.alloc(new Closure(new Function(name, L1, ns), nullptr)));
                int envAddr = nextAddr();
                currentScope->symTable.insert(name, SymbolTableEntry(name, envAddr, constIdx, FUNCVAR, depth(currentScope)+1));
                currentScope = ns;
            }
        }
        void closeScope() {
            if (currentScope != nullptr && currentScope->enclosing != nullptr) {
                currentScope = currentScope->enclosing;
            }
        }
        void insert(string name) {
            currentScope->symTable[name] = SymbolTableEntry(name, nextAddr(), depth(currentScope));
        }
        bool existsInScope(string name) {
            return currentScope->symTable.find(name) != currentScope->symTable.end();
        }
        SymbolTableEntry& lookup(string name) {
            Scope* x = currentScope;
            while (x != nullptr) {
                if (x->symTable.find(name) != x->symTable.end())
                    return x->symTable[name];
                x = x->enclosing;
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