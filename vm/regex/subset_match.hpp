#ifndef subset_match_hpp
#define subset_match_hpp
#include <iostream>
#include <set>
#include "re_compiler.hpp"
using namespace std;

set<NFAState*> move(char ch, set<NFAState*> states) {
    set<NFAState*> next;
    for (NFAState* state : states) {
        for (Transition t : state->transitions) {
            if (t.ch == ch || t.ch == '.') {
                if (!t.is_epsilon && next.find(t.dest) == next.end()) {
                    next.insert(t.dest);
                }
            }
        }
    }
    return next;
}

set<NFAState*> e_closure(set<NFAState*> states) {
    set<NFAState*> next = states;
    Stack<NFAState*> st;
    for (auto s : states) {
        st.push(s);
    }
    while (!st.empty()) {
        NFAState* curr = st.pop();
        for (auto t : curr->transitions) {
            if (t.is_epsilon && next.find(t.dest) == next.end()) {
                next.insert(t.dest);
                st.push(t.dest);
            }
        }
    }
    return next;
}

bool recognizeString(NFA& nfa, string text) {
    set<NFAState*> states = {nfa.start};
    states = e_closure(states);
    int i = 0;
    int matchFrom = 0;
    int matchLen = 0;
    char c;
    for (int i = 0; (c = text[i]) != '\0'; i++) {
        if (states.empty() || c == '\n')
            return matchLen > 0;
        states = e_closure(move(c, states));
        if (states.find(nfa.accept) != states.end()) {
            cout<<"Match Found: ";
            for (int t = matchFrom; t <= i; t++) {
                cout<<text[t];
            }
            cout<<endl;
            matchLen = i - matchFrom;
        }
    }
    return states.find(nfa.accept) != states.end();
}

bool matchRegex(string pattern, string text) {
    NFA nfa = compile(pattern);
    return recognizeString(nfa, text);
}

#endif