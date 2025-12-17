#ifndef compiler_hpp
#define compiler_hpp
#include <iostream>
#include <vector>
#include <stack>
#include "parser.hpp"
using namespace std;

template <class T>
struct Stack : public stack<T> {
    T pop() {
        T ret = stack<T>::top();
        stack<T>::pop();
        return ret;
    }
};

typedef int State;

struct NFAState;

struct Transition {
    char ch;
    bool is_epsilon;
    NFAState* dest;
    Transition(NFAState* d) : ch('&'), dest(d), is_epsilon(true) { }
    Transition(char c, NFAState* d) : ch(c), dest(d), is_epsilon(false) { }
    Transition() {
        is_epsilon = false;
        dest = nullptr;
    }
};

struct NFAState {
    State label;
    vector<Transition> transitions;
    NFAState(State st = -1) : label(st) { }
    ~NFAState() {    }
    bool hasTransition(Transition t) {
        for (auto e : transitions) {
            if (e.ch == t.ch && e.dest == t.dest)
                return true;
        }
        return false;
    }
    void addTransition(Transition t) {
        transitions.push_back(t);
    }
};

struct NFA {
    NFAState* start;
    NFAState* accept;
    NFA(NFAState* s = nullptr, NFAState* a = nullptr) : start(s), accept(a) {  }
};

//about as simple of an allocator as you can get.
const int MAX_STATE = 255;
NFAState arena[MAX_STATE];
int nf = 0;

NFAState* makeState(int label) {
    if (nf+1 == MAX_STATE) {
        cout<<"ERROR: OUT OF MEMORY"<<endl;
        return nullptr;
    }
    NFAState* ns = &arena[nf++];
    ns->label = label;
    return ns;
}

int nextLabel() {
    static int label = 0;
    return label++;
}

NFA makeAtomic(char ch) {
    NFAState* ns = makeState(nextLabel());
    NFAState* ts = makeState(nextLabel());
    ns->addTransition(Transition(ch, ts));
    return NFA(ns, ts);
}

void makeRangeClassTrans(NFAState* ns, NFAState* ts, string ccl, bool negate, int spos) {
    char lo = ccl[spos], hi = ccl[spos+2];
    if (!negate) {
        for (char t = lo; t <= hi; t++)
            ns->addTransition(Transition(t, ts));
        return;
    }
    for (char t = '0'; t < lo; t++)
        ns->addTransition(Transition(t, ts));
    for (char t = hi+1; t <= '~'; t++)
        ns->addTransition(Transition(t, ts));   
}

void makeRegClassTrans(NFAState* ns, NFAState* ts, string ccl, bool negate, int spos) {
    if (!negate) {
        ns->addTransition(Transition(ccl[spos], ts));
        return;
    }
    for (char t = '0'; t <= '~'; t++) {
        if (ccl.find(t) == std::string::npos && !ns->hasTransition(Transition(t, ts))) {
            ns->addTransition(Transition(t, ts));
        }
   }
}

NFA makeCharClass(string ccl) {
    NFAState* ns = makeState(nextLabel());
    NFAState* ts = makeState(nextLabel());
    int i = 0; bool negate = false;
    if (ccl[0] == '^') {
        negate = true; 
        i++;
    }
    while (i < ccl.length()) {
        if (i+2 < ccl.length() && ccl[i+1] == '-') {
            makeRangeClassTrans(ns, ts, ccl, negate, i);
            i += 2;
        } else {
            makeRegClassTrans(ns, ts, ccl, negate, i);
            i++;
        }
    }
    return NFA(ns, ts);
}

// "The empty string"
NFA makeEpsilonAtomic() {
    NFAState* ns = makeState(nextLabel());
    NFAState* ts = makeState(nextLabel());
    ns->addTransition(Transition(ts));
    return NFA(ns, ts);
}

NFA makeConcat(NFA a, NFA b) {
    a.accept->addTransition(Transition(b.start));
    a.accept = b.accept;
    return a;
}

NFA makeAlternate(NFA a, NFA b) {
    NFAState* ns = makeState(nextLabel());
    NFAState* ts = makeState(nextLabel());
    ns->addTransition(Transition(a.start));
    ns->addTransition(Transition(b.start));
    a.accept->addTransition(Transition(ts));
    b.accept->addTransition(Transition(ts));
    return NFA(ns, ts);
}

NFA makeKleene(NFA a, bool must) {
    NFAState* ns = makeState(nextLabel());
    NFAState* ts = makeState(nextLabel());
    ns->addTransition(Transition(a.start));
    if (!must)
        ns->addTransition(Transition(ts));
    a.accept->addTransition(Transition(ts));
    a.accept->addTransition(Transition(a.start));
    return NFA(ns, ts);
}

NFA makeZeorOrOne(NFA a) {
    return makeAlternate(a, makeEpsilonAtomic());
}



class Compiler {
    private:
        Stack<NFA> st;
        void trav(astnode* node) {
            if (node != nullptr) {
                if (node->type == LITERAL) {
                    cout<<"Making Character State: "<<node->c<<endl;
                    st.push(makeAtomic(node->c));
                } else if (node->type == CHCLASS) {
                    st.push(makeCharClass(node->ccl));
                } else {
                    cout<<"Building Operator Machine: "<<node->c<<endl;
                    switch (node->c) {
                        case '|': {
                            trav(node->left);
                            trav(node->right);
                            NFA rhs = st.pop();
                            NFA lhs = st.pop();
                            st.push(makeAlternate(lhs, rhs));
                        } break;
                        case '@': {
                            trav(node->left);
                            trav(node->right);
                            NFA rhs = st.pop();
                            NFA lhs = st.pop();
                            st.push(makeConcat(lhs, rhs));
                        } break;
                        case '*': {
                            trav(node->left);
                            NFA lhs = st.pop();
                            st.push(makeKleene(lhs, false));
                        } break;
                        case '+': {
                            trav(node->left);
                            NFA lhs = st.pop();
                            st.push(makeKleene(lhs, true));
                        } break;
                        case '?': {
                            trav(node->left);
                            NFA lhs = st.pop();
                            st.push(makeZeorOrOne(lhs));
                        } break;
                        default:
                            break;
                    }
                }
            }
        }
    public:
        Compiler() {

        }
        NFA compile(astnode* node) {
            trav(node);
            return st.pop();
        }
};

NFA compile(string pattern) {
    Parser parser;
    astnode* ast = parser.parse(pattern);
    print(ast, 1);
    Compiler compiler;
    return compiler.compile(ast);
}

#endif