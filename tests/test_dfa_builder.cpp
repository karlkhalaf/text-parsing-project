#include "dfa_builder.hpp"
#include "nfa_builder.hpp"
#include <cassert>
#include <string_view>
static void check(const char* pattern, std::string_view text, bool expected) {
    const Nfa nfa = build_nfa_from_pattern(pattern);
    const Dfa dfa = build_dfa_from_nfa(nfa);
    assert(nfa.accepts(text) == expected);
    assert(dfa.accepts(text) == expected);
}
int main() {
    check("a", "", false);
    check("a", "a", true);
    check("a", "b", false);
    check("a|b", "a", true);
    check("a|b", "b", true);
    check("a|b", "ab", false);
    check("(a|b)*", "", true);
    check("(a|b)*", "abba", true);
    check("(a|b)*", "c", false);
    return 0;
}
