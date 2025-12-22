#ifndef compiler_hpp
#define compiler_hpp
#include <iostream>
#include <vector>
#include <stack>
using namespace std;

const int LITERAL = 1;
const int OPERATOR = 2;
const int CHCLASS = 3;

struct re_ast {
    int type;
    char c;
    string ccl;
    re_ast* left;
    re_ast* right;
    re_ast(string cl, int t) : type(t), c('['), ccl(cl), left(nullptr), right(nullptr) { }
    re_ast(char ch, int t) : type(t), c(ch), ccl(""), left(nullptr), right(nullptr) { }
};

void preorder(re_ast* t, int d) {
    d++;
    if (t != nullptr) {
        for (int i = 0; i < d; i++) cout<<" ";
        if (t->type == 1 || t->type == 2)
            cout<<t->c<<endl;
        else cout<<t->ccl<<endl;
        preorder(t->left, d+1);
        preorder(t->right, d+1);
    }
    d--;
}


class REParser {
    private:
        string rexpr;
        int pos;
        void advance() {
            if (pos < rexpr.length())
                pos++;
        }
        bool match(char c) {
            if (c == rexpr[pos]) {
                advance();
                return true;
            }
            return false;
        }
        char lookahead() {
            return rexpr[pos];
        }
        re_ast* factor() {
            re_ast* t;
            if (lookahead() == '(') {
                match('(');
                t = anchordexprs();
                match(')');
            } else if (isdigit(lookahead()) || isalpha(lookahead()) || lookahead() == '.') {
                t = new re_ast(lookahead(), 1);
                advance();
            } else if (lookahead() == '[') {
                advance();
                string ccl;
                while (pos+1 < rexpr.length() && lookahead() != ']') {
                    ccl.push_back(lookahead());
                    advance();
                }
                if (lookahead() != ']') {
                    cout<<"Error: Unclosed character class."<<endl;
                    return nullptr;
                } else {
                    advance();
                }
                t = new re_ast(ccl, 3);
            }

            if (lookahead() == '*' || lookahead() == '+' || lookahead() == '?') {
                re_ast* n = new re_ast(lookahead(), 2);
                match(lookahead());
                n->left = t;
                t = n;
            }
            return t;
        }
        re_ast* term() {
            re_ast* t = factor();
            if (lookahead() == '(' || (lookahead() == '.' || isdigit(lookahead()) || isalpha(lookahead()) || lookahead() == '[')) {
                re_ast* n = new re_ast('@', 2);
                n->left = t;
                n->right = term();
                t = n;
            }
            return t;
        }
        re_ast* expr() {
            re_ast* t = term();
            if (lookahead() == '|') {
                re_ast* n = new re_ast('|', 2);
                match('|');
                n->left = t;
                n->right = expr();
                t = n;
            }
            return t;
        }
        re_ast* anchordexprs() {
            re_ast* t = nullptr;
            if (lookahead() == '^') {
                t = new re_ast('^', 2);
                advance();
                t->left = expr();
            } else {
                t = expr();
            }
            if (t != nullptr && lookahead() == '$') {
                advance();
                if (t->c == '^')
                    t->right = new re_ast('$', 2);
                else {
                    re_ast* n = new re_ast('$', 2);
                    n->left = t;
                    t = n;
                }
            }
            return t;
        }
    public:
        REParser() {

        }
        re_ast* parse(string pat) {
            rexpr = pat; pos = 0;
            return anchordexprs();
        }
};

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



class RECompiler {
    private:
        Stack<NFA> st;
        void trav(re_ast* node) {
            if (node != nullptr) {
                if (node->type == LITERAL) {
                    //cout<<"Making Character State: "<<node->c<<endl;
                    st.push(makeAtomic(node->c));
                } else if (node->type == CHCLASS) {
                    st.push(makeCharClass(node->ccl));
                } else {
                    //cout<<"Building Operator Machine: "<<node->c<<endl;
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
        RECompiler() {

        }
        NFA compile(re_ast* node) {
            trav(node);
            return st.pop();
        }
};

NFA compile(string pattern) {
    REParser parser;
    re_ast* ast = parser.parse(pattern);
    preorder(ast, 1);
    RECompiler compiler;
    return compiler.compile(ast);
}

#endif