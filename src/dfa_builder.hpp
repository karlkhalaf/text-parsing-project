#ifndef DFA_BUILDER_HPP
#define DFA_BUILDER_HPP
#include "dfa.hpp"
#include "nfa.hpp"
Dfa build_dfa_from_nfa(const Nfa& nfa);
#endif
