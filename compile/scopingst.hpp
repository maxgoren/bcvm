#ifndef scopingst_hpp
#define scopingst_hpp
#include "../vm/stackitem.hpp"
#include <map>
#include <vector>
#include <iostream>
using namespace std;

struct Scope;

struct STItem {
    string name;
    int type;
    int addr;
    int depth;
    Function* func;
    STItem(string n, int adr, int sip, Scope* s, int d) : type(2), name(n), depth(d), addr(adr), func(new Function(n, sip, s)) { }
    STItem(string n, int adr, int d) : type(1), name(n), addr(adr), depth(d), func(nullptr) { }
    STItem() : type(0), func(nullptr) { }
};

struct Scope {
    Scope* enclosing;
    map<string, STItem> symTable;
};


class ScopingST {
    private:
        Scope* st;
        int nextAddr() {
            return st->symTable.size()+1; //locals[0] is resrved for return value.
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
            return d;
        }
    public:
        ScopingST() {
            st = new Scope();
            st->enclosing = nullptr;
        }
        void openFunctionScope(string name, int L1) {
            if (st->symTable.find(name) != st->symTable.end()) {
                Scope* ns = st->symTable[name].func->scope;
                ns->enclosing = st;
                st->symTable[name].func->start_ip = L1;
                st = ns;
            } else {
                Scope*  ns = new Scope;
                ns->enclosing = st;
                st->symTable.insert(make_pair(name,STItem(name, nextAddr(), L1, ns, depth(ns))));
                st = ns;
            }
        }
        void copyScope(string dest, string src) {
            if (st->symTable.find(src) != st->symTable.end()) {
                STItem item = st->symTable[src];
                if (item.type == 2) {
                    Function* ns = item.func;
                    st->symTable[dest] = STItem(dest, nextAddr(), ns->start_ip, ns->scope, depth(ns->scope));
                }
            } 
        }
        void closeScope() {
            if (st->enclosing != nullptr) {
                st = st->enclosing;
            }
        }
        void insert(string name) {
            st->symTable[name] = STItem(name, nextAddr(), depth(st));
        }
        STItem lookup(string name) {
            Scope* x = st;
            while (x != nullptr) {
                if (x->symTable.find(name) != x->symTable.end())
                    return x->symTable[name];
                x = x->enclosing;
            }
            return STItem("not found", -1, -1);
        }
        int depth() {
            return depth(st);
        }
        void printScope(Scope* s, int d) {
            auto x = s;
            for (auto m : x->symTable) {
                for (int i = 0; i < d; i++) cout<<"  ";
                cout<<m.first<<": "<<m.second.addr<<", "<<m.second.depth<<endl;
                if (m.second.type == 2) {
                    printScope(m.second.func->scope,d + 1);
                }
            }
        }
        void print() {
            printScope(st, 1);
        }
};

#endif